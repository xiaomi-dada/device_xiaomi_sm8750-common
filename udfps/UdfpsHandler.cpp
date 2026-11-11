/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.xiaomi_sm8750"

#include <android-base/file.h>
#include <android-base/logging.h>

#include <cstring>

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
constexpr const char* kQcomModuleName = "Qcom Fingerprint HAL";

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
        LOG(INFO) << "HAL module: " << (moduleName ? moduleName : "unknown");
    }

    void onFingerDown(uint32_t x, uint32_t y, float /*minor*/, float /*major*/) override {
        if (!mDevice) {
            return;
        }

        LOG(DEBUG) << __func__ << " x: " << x << ", y: " << y;
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
        setFingerDown(false);
        mDevice->extCmd(mDevice, kCommandFodPressX, 0);
        mDevice->extCmd(mDevice, kCommandFodPressY, 0);
        mDevice->extCmd(mDevice, kCommandFodPressStatus, kParamFodReleased);
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
