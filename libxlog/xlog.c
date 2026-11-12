/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

// Xiaomi's xlog hands audio events to MIUI's media event gathering service.
// There is no such service here, so logging is never enabled.

struct xlog_msg;

int xlog_check_permission(const char* name, int level) {
    (void)name;
    (void)level;
    return 0;
}

int xlog_send(int type, const struct xlog_msg* msg) {
    (void)type;
    (void)msg;
    return 0;
}

int xlog_send_int(int type, const struct xlog_msg* msg) {
    (void)type;
    (void)msg;
    return 0;
}
