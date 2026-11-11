/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.xiaomi_sm8750"

#include <android-base/file.h>
#include <android-base/logging.h>

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

// intr2 is a GPIO from the SoC to the ultrasonic sensor, whose firmware only
// reports a finger as present while it is high. The kernel drives it from
// touch events while the sensor waits for a finger, but the HAL module hands
// it over to userspace for enrollment and authentication.
constexpr const char* kIntr2Path = "/sys/bus/platform/devices/soc:xiaomi_fingerprint/intr2";
// Lets the display know a finger is on the sensor.
constexpr const char* kFingerDownPath = "/sys/bus/platform/devices/soc:xiaomi_fingerprint/fingerdown";

void writeFingerState(bool down) {
    for (const char* path : {kIntr2Path, kFingerDownPath}) {
        if (!android::base::WriteStringToFile(down ? "1" : "0", path)) {
            PLOG(ERROR) << "Failed to write " << path;
        }
    }
}

}  // namespace

class XiaomiSM8750UdfpsHandler : public UdfpsHandler {
  public:
    void init(fingerprint_device_t* device) override { mDevice = device; }

    void onFingerDown(uint32_t x, uint32_t y, float /*minor*/, float /*major*/) override {
        LOG(DEBUG) << __func__ << " x: " << x << ", y: " << y;
        writeFingerState(true);
        mDevice->extCmd(mDevice, kCommandFodPressX, x);
        mDevice->extCmd(mDevice, kCommandFodPressY, y);
        mDevice->extCmd(mDevice, kCommandFodPressStatus, kParamFodPressed);
    }

    // The module takes its reference image of the empty sensor when the finger
    // is released, so only a real release from the touchscreen ends a press.
    void onFingerUp() override {
        LOG(DEBUG) << __func__;
        writeFingerState(false);
        mDevice->extCmd(mDevice, kCommandFodPressX, 0);
        mDevice->extCmd(mDevice, kCommandFodPressY, 0);
        mDevice->extCmd(mDevice, kCommandFodPressStatus, kParamFodReleased);
    }

  private:
    fingerprint_device_t* mDevice = nullptr;
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
