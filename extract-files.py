#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/xiaomi/sm8750-common',
    'hardware/qcom-caf/sm8750',
    'hardware/qcom-caf/wlan',
    'hardware/xiaomi',
    'vendor/qcom/opensource/commonsys/display',
    'vendor/qcom/opensource/commonsys-intf/display',
    'vendor/qcom/opensource/dataservices',
]


def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'vendor' else None


lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    (
        'vendor.qti.ImsRtpService-V1-ndk',
        'vendor.qti.diaghal-V1-ndk',
        'vendor.qti.hardware.dpmaidlservice-V1-ndk',
        'vendor.qti.hardware.wifidisplaysession_aidl-V1-ndk',
        'vendor.qti.qccsyshal_aidl-V1-ndk',
        'vendor.qti.qccvndhal_aidl-V1-ndk',
    ): lib_fixup_vendor_suffix,
}

# Xiaomi built these frozen audio interfaces against their own
# android.media.audio.common.types-V6, so their parcelable layouts differ from
# the ones generated from source. The prebuilt audio stack ships and links
# Xiaomi's copies under a _prebuilt name.
xiaomi_audio_aidl_libs = (
    'android.hardware.audio.common-V3-ndk',
    'android.hardware.audio.core-V2-ndk',
    'android.hardware.audio.core.sounddose-V2-ndk',
    'android.hardware.audio.effect-V2-ndk',
    'android.hardware.bluetooth.audio-V4-ndk',
)


def xiaomi_audio_aidl_fixup() -> blob_fixup:
    fixup = blob_fixup()
    for lib in xiaomi_audio_aidl_libs:
        fixup.replace_needed(f'{lib}.so', f'{lib}_prebuilt.so')
    return fixup


blob_fixups: blob_fixups_user_type = {
    # Xiaomi's libaudioroute_ext reads audio_route's internal structures,
    # whose layout changed in Android 16. Keep the whole prebuilt audio route
    # stack, which shares audio_route objects, on the Android 14 library.
    (
        'odm/lib64/libaudioroute_ext.so',
        'vendor/lib64/libagm.so',
        'vendor/lib64/libar-pal.so',
        'vendor/lib64/libmcs.so',
    ): blob_fixup()
        .replace_needed('libaudioroute.so', 'libaudioroute-v34.so'),
    (
        'vendor/lib64/android.hardware.audio.common-V3-ndk_prebuilt.so',
        'vendor/lib64/android.hardware.audio.core-V2-ndk_prebuilt.so',
        'vendor/lib64/android.hardware.audio.core.sounddose-V2-ndk_prebuilt.so',
        'vendor/lib64/android.hardware.audio.effect-V2-ndk_prebuilt.so',
        'vendor/lib64/android.hardware.bluetooth.audio-V4-ndk_prebuilt.so',
        'vendor/lib64/btaudio_offload_if.so',
        'vendor/lib64/hw/android.hardware.bluetooth.audio-impl-qti.so',
        'vendor/lib64/hw/android.hardware.bluetooth.audio_sw.so',
        'vendor/lib64/hw/audio.bluetooth_qti.default.so',
        'vendor/lib64/hw/libaudiocorehal.default.so',
        'vendor/lib64/hw/libaudiocorehal.qti.so',
        'vendor/lib64/hw/libaudioeffecthal.qti.so',
        'vendor/lib64/libaudio_aidl_conversion_common_ndk_prebuilt.so',
        'vendor/lib64/libaudioplatformconverter.qti.so',
        'vendor/lib64/libbluetooth_audio_session_aidl_prebuilt.so',
        'vendor/lib64/libbluetooth_audio_session_aidl_qti.so',
        'vendor/lib64/libqtigefar.so',
        'vendor/lib64/soundfx/libdownmixaidl.so',
        'vendor/lib64/soundfx/libdynamicsprocessingaidl.so',
        'vendor/lib64/soundfx/libloudnessenhanceraidl.so',
        'vendor/lib64/soundfx/libqcompostprocbundle.so',
        'vendor/lib64/soundfx/libqcomvisualizer.so',
        'vendor/lib64/soundfx/libqcomvoiceprocessing.so',
        'vendor/lib64/soundfx/libquasar.so',
        'vendor/lib64/soundfx/libreverbaidl.so',
        'vendor/lib64/soundfx/libvisualizeraidl.so',
        'vendor/lib64/soundfx/libvolumelistener.so',
    ): xiaomi_audio_aidl_fixup(),
    'system_ext/lib64/libwfdnative.so': blob_fixup()
        .add_needed('libinput_shim.so'),
    'vendor/etc/media_codecs_sun.xml': blob_fixup()
        .regex_replace('.*media_codecs_(google_audio|google_c2|google_telephony|google_video|vendor_audio).*\n', ''),
    'vendor/etc/seccomp_policy/gnss@2.0-qsap-location.policy': blob_fixup()
        .add_line_if_missing('sched_get_priority_min: 1')
        .add_line_if_missing('sched_get_priority_max: 1'),
    'vendor/lib64/android.hardware.bluetooth.audio-impl_prebuilt.so': xiaomi_audio_aidl_fixup()
        .replace_needed('libbluetooth_audio_session_aidl.so', 'libbluetooth_audio_session_aidl_prebuilt.so'),
    (
        'vendor/lib64/libVoiceSdk.so',
        'vendor/lib64/libcapiv2uvvendor.so',
        'vendor/lib64/liblistensoundmodel2vendor.so',
    ): blob_fixup()
        .replace_needed('libtensorflowlite_c.so', 'libtensorflowlite_c_vendor.so'),
    'vendor/lib64/libqcodec2_core.so': blob_fixup()
        .replace_needed('android.hardware.graphics.common-V5-ndk.so', 'android.hardware.graphics.common-V7-ndk.so'),
    'vendor/lib64/libgnss.so': blob_fixup()
        .replace_needed('android.hardware.sensors-V2-ndk.so', 'android.hardware.sensors-V3-ndk.so'),
    'vendor/lib64/soundfx/libbundleaidl.so': xiaomi_audio_aidl_fixup()
        .replace_needed('libaudio_aidl_conversion_common_ndk.so', 'libaudio_aidl_conversion_common_ndk_prebuilt.so'),
    'vendor/lib64/libaudioserviceexampleimpl.so': xiaomi_audio_aidl_fixup()
        .add_needed('libaudioutils_shim.so')
        .replace_needed('android.hardware.bluetooth.audio-impl.so', 'android.hardware.bluetooth.audio-impl_prebuilt.so')
        .replace_needed('libaudio_aidl_conversion_common_ndk.so', 'libaudio_aidl_conversion_common_ndk_prebuilt.so')
        .replace_needed('libbluetooth_audio_session_aidl.so', 'libbluetooth_audio_session_aidl_prebuilt.so'),
    'vendor/lib64/libqti-perfd.so': blob_fixup()
        .replace_needed('vendor.qti.hardware.display.config-V5-ndk.so', 'vendor.qti.hardware.display.config-V12-ndk.so'),
    'vendor/lib64/libwfdmmsrc_proprietary.so': xiaomi_audio_aidl_fixup()
        .replace_needed('android.media.audio.common.types-V2-ndk.so', 'android.media.audio.common.types-V3-ndk.so'),
}  # fmt: skip

module = ExtractUtilsModule(
    'sm8750-common',
    'xiaomi',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
    check_elf=True,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
