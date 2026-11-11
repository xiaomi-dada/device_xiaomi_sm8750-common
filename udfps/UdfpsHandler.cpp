/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.xiaomi_sm8750"

#include <aidl/android/hardware/biometrics/fingerprint/AcquiredInfo.h>
#include <android-base/file.h>
#include <android-base/logging.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>

#include "UdfpsHandler.h"

namespace {

// Commands of the Xiaomi fingerprint HAL modules. The touchscreen, not the
// sensor, detects the finger, so the module is told where and when it lands
// on the sensor area.
constexpr int32_t kCommandFodPressStatus = 1;
constexpr int32_t kCommandFodPressX = 2;
constexpr int32_t kCommandFodPressY = 3;

constexpr int32_t kParamFodReleased = 0;
constexpr int32_t kParamFodPressed = 1;

// The device is sold with either a Qualcomm (QFS4008) or a Goodix ultrasonic
// sensor, each driven by its own HAL module.
constexpr const char* kGoodixModuleName = "Goodix Fingerprint HAL";
constexpr const char* kQcomModuleName = "Qcom Fingerprint HAL";

// The Goodix HAL module reports a capture that failed on a corrupted SPI
// transfer with this vendor acquired code while the finger is still down, and
// does not retry it. Ask for another capture of the same press, a bounded
// number of times.
constexpr int32_t kGoodixVendorCodeSpiCrcFailed = 41;
constexpr int kGoodixCaptureRetries = 2;
constexpr auto kGoodixCaptureRetryDelay = std::chrono::milliseconds(50);
// A capture takes longer than this, so a failure reported sooner after a press
// belongs to the previous one.
constexpr auto kGoodixMinCaptureTime = std::chrono::milliseconds(50);

// Lets the fingerprint driver and the display know a finger is on the sensor.
constexpr const char* kFingerDownPath = "/sys/bus/platform/devices/soc:xiaomi_fingerprint/fingerdown";
// GPIO from the SoC to the sensor. The Qualcomm sensor firmware only reports a
// finger as present while it is high: the kernel drives it from touch events
// while the sensor waits for a finger, but the HAL module hands it over to
// userspace for enrollment and authentication. On the Goodix sensor the same
// GPIO is the reset line, so it must not be touched there.
constexpr const char* kIntr2Path = "/sys/bus/platform/devices/soc:xiaomi_fingerprint/intr2";

void writeNode(const char* path, bool value) {
    if (!android::base::WriteStringToFile(value ? "1" : "0", path)) {
        PLOG(ERROR) << "Failed to write " << path;
    }
}

using ::aidl::android::hardware::biometrics::fingerprint::AcquiredInfo;

}  // namespace

class XiaomiSM8750UdfpsHandler : public UdfpsHandler {
  public:
    void init(fingerprint_device_t* device) override {
        mDevice = device;
        if (!mDevice) {
            return;
        }

        const char* moduleName = mDevice->common.module->name;
        mDriveIntr2 = moduleName && strcmp(moduleName, kQcomModuleName) == 0;
        mRetryCrcFailures = moduleName && strcmp(moduleName, kGoodixModuleName) == 0;
        LOG(INFO) << "HAL module: " << (moduleName ? moduleName : "unknown");
    }

    void onFingerDown(uint32_t x, uint32_t y, float /*minor*/, float /*major*/) override {
        if (!mDevice) {
            return;
        }

        LOG(DEBUG) << __func__ << " x: " << x << ", y: " << y;
        std::lock_guard lock(mCommandMutex);
        mFingerDown = true;
        mPressId++;
        mPressTime = std::chrono::steady_clock::now();
        mCaptureRetries = 0;
        setFingerDown(true);
        mDevice->extCmd(mDevice, kCommandFodPressX, x);
        mDevice->extCmd(mDevice, kCommandFodPressY, y);
        mDevice->extCmd(mDevice, kCommandFodPressStatus, kParamFodPressed);
    }

    // The module takes its reference image of the empty sensor when the finger
    // is released, so only a real release from the touchscreen ends a press.
    void onFingerUp() override {
        if (!mDevice) {
            return;
        }

        LOG(DEBUG) << __func__;
        std::lock_guard lock(mCommandMutex);
        mFingerDown = false;
        mPressId++;
        setFingerDown(false);
        mDevice->extCmd(mDevice, kCommandFodPressX, 0);
        mDevice->extCmd(mDevice, kCommandFodPressY, 0);
        mDevice->extCmd(mDevice, kCommandFodPressStatus, kParamFodReleased);
    }

    // Runs in the HAL module's notify callback. The retry waits for the sensor to
    // recover from the failed transfer, so it is sent from its own thread.
    void onAcquired(int32_t result, int32_t vendorCode) override {
        const uint32_t pressId = mPressId;
        if (!mRetryCrcFailures || result != static_cast<int32_t>(AcquiredInfo::VENDOR) ||
            vendorCode != kGoodixVendorCodeSpiCrcFailed || !mFingerDown ||
            std::chrono::steady_clock::now() - mPressTime.load() < kGoodixMinCaptureTime ||
            mCaptureRetries.fetch_add(1) >= kGoodixCaptureRetries) {
            return;
        }

        LOG(INFO) << "Capture failed on a SPI CRC error, retrying";
        std::thread([this, pressId] {
            std::this_thread::sleep_for(kGoodixCaptureRetryDelay);
            std::lock_guard lock(mCommandMutex);
            if (mFingerDown && mPressId == pressId) {
                mDevice->extCmd(mDevice, kCommandFodPressStatus, kParamFodPressed);
            }
        }).detach();
    }

  private:
    void setFingerDown(bool down) {
        if (mDriveIntr2) {
            writeNode(kIntr2Path, down);
        }
        writeNode(kFingerDownPath, down);
    }

    fingerprint_device_t* mDevice = nullptr;
    bool mDriveIntr2 = false;
    bool mRetryCrcFailures = false;

    // Serializes the commands of a press with those of capture retries, which
    // are sent from their own thread.
    std::mutex mCommandMutex;
    std::atomic<bool> mFingerDown = false;
    std::atomic<uint32_t> mPressId = 0;
    std::atomic<std::chrono::steady_clock::time_point> mPressTime;
    std::atomic<int> mCaptureRetries = 0;
};

static UdfpsHandler* create() {
    return new XiaomiSM8750UdfpsHandler();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
        .create = create,
        .destroy = destroy,
};
