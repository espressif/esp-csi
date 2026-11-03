/*
 * SPDX-FileCopyrightText: 2026 vb3227
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_idf_version.h>
#include <esp_wifi.h>
#include <stddef.h>
#include <string.h>
#include <atomic>

// esp-radar 0.2.0 ESP32-S3 archive reads dmac at 54, buf at 64 and len at 68.
// K10's IDF 4.4.4 driver has no dmac and passes buf at 56 and len at 60.
// Symbol wrapping converts the driver's callback argument, without changing SDK.
#if !CONFIG_IDF_TARGET_ESP32S3 || ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(4, 4, 4)
#error Re-audit the esp-radar binary CSI ABI before using a different SDK or target
#endif

namespace {
struct RadarCsiInfo {
    wifi_pkt_rx_ctrl_t rx_ctrl;
    uint8_t mac[6];
    uint8_t dmac[6];
    bool first_word_invalid;
    int8_t *buf;
    uint16_t len;
};
static_assert(sizeof(wifi_pkt_rx_ctrl_t) == 48, "Unexpected receive metadata ABI");
static_assert(offsetof(wifi_csi_info_t, buf) == 56, "Unexpected SDK CSI buffer offset");
static_assert(offsetof(wifi_csi_info_t, len) == 60, "Unexpected SDK CSI length offset");
static_assert(offsetof(RadarCsiInfo, dmac) == 54, "Unexpected radar dmac offset");
static_assert(offsetof(RadarCsiInfo, first_word_invalid) == 60, "Unexpected radar invalid-word offset");
static_assert(offsetof(RadarCsiInfo, buf) == 64, "Unexpected radar buffer offset");
static_assert(offsetof(RadarCsiInfo, len) == 68, "Unexpected radar length offset");
std::atomic<wifi_csi_cb_t> radarCallback{nullptr};
std::atomic<void *> radarContext{nullptr};

constexpr RadarCsiInfo translate(const wifi_csi_info_t &info)
{
    return {info.rx_ctrl,
        {info.mac[0], info.mac[1], info.mac[2], info.mac[3], info.mac[4], info.mac[5]},
        {0, 0, 0, 0, 0, 0}, info.first_word_invalid, info.buf, info.len};
}

// Compile-time regression tests exercise the actual conversion used below.
int8_t probePayload[4];
constexpr wifi_csi_info_t probe{{-42}, {1, 2, 3, 4, 5, 6}, true, probePayload, 4};
static_assert(translate(probe).buf == probePayload, "CSI payload pointer must be preserved");
static_assert(translate(probe).len == 4, "CSI length must be preserved");
static_assert(translate(probe).first_word_invalid, "Invalid first-word flag must be preserved");
static_assert(translate(probe).mac[0] == 1 && translate(probe).mac[5] == 6, "Source MAC must be preserved");
static_assert(translate(probe).rx_ctrl.rssi == -42, "Radio metadata must be preserved");
static_assert(translate(probe).dmac[0] == 0 && translate(probe).dmac[5] == 0, "Unknown dmac must be zero");

void driverCallback(void *, wifi_csi_info_t *info)
{
    auto callback = radarCallback.load();
    if (!callback || !info || !info->buf || info->len < 4) {
        return;
    }
    // This SDK supplies no destination MAC. Radar's destination filter is disabled.
    RadarCsiInfo translated = translate(*info);
    callback(radarContext.load(), reinterpret_cast<wifi_csi_info_t *>(&translated));
}
} // namespace

extern "C" esp_err_t __real_esp_wifi_set_csi_rx_cb(wifi_csi_cb_t, void *);
extern "C" esp_err_t __wrap_esp_wifi_set_csi_rx_cb(wifi_csi_cb_t callback, void *context)
{
    auto previousCallback = radarCallback.load();
    void *previousContext = radarContext.load();
    radarContext.store(context); radarCallback.store(callback);
    esp_err_t result = __real_esp_wifi_set_csi_rx_cb(callback ? driverCallback : nullptr, nullptr);
    if (result != ESP_OK) {
        radarContext.store(previousContext); radarCallback.store(previousCallback);
    }
    return result;
}
