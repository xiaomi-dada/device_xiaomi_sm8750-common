/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

// Xiaomi's MiSight reports events from the vendor blobs to MIUI's analytics
// service. There is no such service here, so the events are dropped.

#include <cstdarg>
#include <cstdint>
#include <string>
#include <vector>

namespace android {

class MiEvent {
  public:
    explicit MiEvent(uint32_t id);
    MiEvent(const MiEvent& other);
    virtual ~MiEvent();

    bool putInt(const std::string& key, int value);
    bool putLong(const std::string& key, long value);
    bool putString(const std::string& key, const std::string& value);
    bool putString(const std::string& key, const std::vector<std::string>& value);
    int report();

  private:
    // The blobs allocate MiEvent themselves, so it keeps the stock size.
    [[maybe_unused]] uint32_t mId;
    [[maybe_unused]] uint32_t mTimestamp = 0;
    [[maybe_unused]] void* mPayload = nullptr;
};

static_assert(sizeof(MiEvent) == 24);

MiEvent::MiEvent(uint32_t id) : mId(id) {}

MiEvent::MiEvent(const MiEvent& other) = default;

MiEvent::~MiEvent() {}

bool MiEvent::putInt(const std::string&, int) {
    return true;
}

bool MiEvent::putLong(const std::string&, long) {
    return true;
}

bool MiEvent::putString(const std::string&, const std::string&) {
    return true;
}

bool MiEvent::putString(const std::string&, const std::vector<std::string>&) {
    return true;
}

int MiEvent::report() {
    return 0;
}

namespace MiSight {

int sendEvent(MiEvent& event) {
    return event.report();
}

}  // namespace MiSight

}  // namespace android

extern "C" int sendEventMisight(int, const char*) {
    return 0;
}

extern "C" int sendEventMisightF(int, const char*, ...) {
    return 0;
}
