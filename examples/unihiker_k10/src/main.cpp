/*
 * SPDX-FileCopyrightText: 2026 vb3227
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>
#include <WiFi.h>
#include <unihiker_k10.h>
#include <esp_idf_version.h>
#include <esp_log.h>
#include <freertos/queue.h>
#include <esp_radar.h>
#include <ping/ping_sock.h>
#include <atomic>
#include <cmath>
#include "board_config.h"
#include "screen_text.h"
#include "original_decision.h"

UNIHIKER_K10 k10;
namespace {
constexpr uint32_t CALIBRATION_MS = 15000, DATA_TIMEOUT_MS = 1500;
struct Result {
    float wander, jitter;
    uint32_t at;
};
QueueHandle_t results = nullptr;
std::atomic<uint32_t> rawCount{0}, rawAt{0};
std::atomic<int> rawRssi{0};
std::atomic<uint32_t> pingReplies{0}, pingTimeouts{0}, disconnectReason{0};
esp_ping_handle_t pingSession = nullptr;
bool initialized = false, running = false, training = false, ready = false;
bool configValid = false, motion = false, presence = false;
bool pendingTraining = false;
uint32_t leaveAt = 0;
unsigned sensitivityProfile = 1; // 0: original, 1: high, 2: very high
OriginalDecision decision;
std::atomic<uint32_t> droppedReports{0};
uint32_t trainAt = 0, trainCount = 0, reportAt = 0, reportCount = 0;
uint32_t lastAttempt = 0, lastDraw = 0, buttonChanged = 0;
float wanderLimit = 0, jitterLimit = 0, activity = 0;

String error;

// Keep our explicit Serial diagnostics. The closed-source archive's verbose
// printf logging crashes in this SDK's formatting path during calibration.
int discardNativeLog(const char *, va_list)
{
    return 0;
}

void pingSuccess(esp_ping_handle_t, void *)
{
    pingReplies.fetch_add(1);
}
void pingTimeout(esp_ping_handle_t, void *)
{
    pingTimeouts.fetch_add(1);
}

bool check(esp_err_t rc, const char *stage)
{
    if (rc == ESP_OK) {
        return true;
    }
    error = String(stage) + ": " + esp_err_to_name(rc);
    Serial.println(error); return false;
}

void rawCallback(const wifi_csi_filtered_info_t *info, void *)
{
    if (!info || info->valid_len <= 0) {
        return;
    }
    rawRssi.store(info->rx_ctrl.rssi);
    rawAt.store(millis()); rawCount.fetch_add(1);
}

void radarCallback(const wifi_radar_info_t *info, void *)
{
    if (!info || !results || !std::isfinite(info->waveform_wander) ||
            !std::isfinite(info->waveform_jitter) || info->waveform_wander < 0 || info->waveform_jitter < 0) {
        return;
    }
    Result result{info->waveform_wander, info->waveform_jitter, millis()};
    // Native radar worker: copy only. Display and calibration run in loop().
    if (xQueueSend(results, &result, 0) != pdTRUE) {
        droppedReports.fetch_add(1);
    }
}

void clearDetection()
{
    ready = motion = presence = false;
    decision.clear(); activity = 0;
    reportAt = reportCount = 0;
}

esp_err_t cancelTraining()
{
    float ignoredWander = 0, ignoredJitter = 0;
    return esp_radar_train_stop(&ignoredWander, &ignoredJitter);
}

bool startTraining()
{
    clearDetection();
    if (training && !check(cancelTraining(), "Cancel training")) {
        return false;
    }
    training = false; pendingTraining = true; leaveAt = millis();
    decision.presenceKnown = false;
    error = "";
    Serial.println("CALIBRATION_DELAY: leave room within 10 seconds, then keep it still for 15 seconds");
    return true;
}

bool beginTraining()
{
    if (!check(esp_radar_train_remove(), "Clear baseline") ||
            !check(esp_radar_train_start(), "Start training")) {
        return false;
    }
    pendingTraining = false; training = true;
    trainAt = millis(); trainCount = rawCount.load();
    reportCount = reportAt = 0; xQueueReset(results);
    Serial.println("CALIBRATION: collecting native Espressif baseline");
    return true;
}

void stopCapture()
{
    if (training) {
        cancelTraining();
        training = false;
    }
    if (running) {
        esp_radar_stop();
    }
    if (pingSession) {
        esp_ping_stop(pingSession); esp_ping_delete_session(pingSession); pingSession = nullptr;
    }
    running = false; pendingTraining = false; clearDetection(); rawAt.store(0);
    if (results) {
        xQueueReset(results);
    }
}

bool startCapture()
{
    if (!initialized) {
        if (!check(esp_radar_init(), "Radar init")) {
            return false;
        }
        initialized = true;
    }
    wifi_ap_record_t ap{};
    if (!check(esp_wifi_sta_get_ap_info(&ap), "AP") ||
            !check(esp_wifi_set_ps(WIFI_PS_NONE), "Power save")) {
        return false;
    }
    // Same settings as Espressif 0.2.0 defaults, built field by field for C++.
    wifi_radar_config_t config{};
    memcpy(config.filter_mac, ap.bssid, 6);
    // IDF 4.4 has no destination MAC in CSI metadata; keep source filtering only.
    memset(config.filter_dmac, 0xff, sizeof(config.filter_dmac));
    config.wifi_radar_cb = radarCallback;
    config.wifi_csi_filtered_cb = rawCallback;
    // Original Espressif default; preserved S3 metadata includes RF gain bits.
    config.wifi_radar_compensate_en = true;
    config.csi_handle_priority = configMAX_PRIORITIES - 1; config.csi_combine_priority = configMAX_PRIORITIES - 1;
    config.csi_recv_interval = 10; config.csi_handle_time = 250;
    config.csi_config.lltf_en = true;
    config.csi_config.manu_scale = true; config.csi_config.shift = 4;
    if (!check(esp_radar_set_config(&config), "Radar config") ||
            !check(esp_radar_start(), "Radar start")) {
        return false;
    }
    running = true;
    esp_ping_config_t ping = ESP_PING_DEFAULT_CONFIG();
    ping.count = 0; ping.interval_ms = 10; ping.timeout_ms = 100; ping.data_size = 1;
    ping.target_addr.type = ESP_IPADDR_TYPE_V4;
    ping.target_addr.u_addr.ip4.addr = static_cast<uint32_t>(WiFi.gatewayIP());
    esp_ping_callbacks_t callbacks{};
    callbacks.on_ping_success = pingSuccess;
    callbacks.on_ping_timeout = pingTimeout;
    pingReplies.store(0); pingTimeouts.store(0);
    if (!check(esp_ping_new_session(&ping, &callbacks, &pingSession), "Ping create") ||
            !check(esp_ping_start(pingSession), "Ping start")) {
        return false;
    }
    return startTraining();
}

void process(const Result &result)
{
    if (millis() - result.at >= DATA_TIMEOUT_MS) {
        return;
    }
    reportAt = result.at; ++reportCount;
    if (training || pendingTraining || !ready) {
        return;
    }
    if (decision.push(result.wander, result.jitter)) {
        motion = decision.movement;
        presence = decision.presence;
        activity = decision.activity;
    }
}

void finishTraining()
{
    float wander = 0, jitter = 0;
    bool ok = check(esp_radar_train_stop(&wander, &jitter), "Finish training");
    training = false;
    if (!ok) {
        clearDetection();
        return;
    }
    decision.baseline(wander, jitter);
    wanderLimit = decision.wanderThreshold; jitterLimit = decision.movementThreshold;
    ready = true;
    Serial.printf("BASELINE,native_wander=%.8f,native_jitter=%.8f,presence_valid=%d,motion_threshold=%.8f\n",
                  wander, jitter, decision.presenceKnown, jitterLimit);
    if (!decision.presenceKnown) {
        Serial.println("MODE: motion only; native presence calibration invalid, no fabricated threshold");
    }
}

void line(int x1, int y1, int x2, int y2, uint32_t color)
{
    k10.canvas->canvasLine(x1, y1, x2, y2, color);
}

void draw()
{
    uint32_t received = rawAt.load(), now = millis();
    bool fresh = running && received && now - received < DATA_TIMEOUT_MS &&
                 reportAt && now - reportAt < DATA_TIMEOUT_MS;
    const char *status = error.length() ? "ERROR: SEE SERIAL" : !running ? "CONNECTING WI-FI" :
                         pendingTraining ? "LEAVE ROOM: 10s" : !fresh ? "WAITING FOR CSI" : training ? "CALIBRATING" : !ready ? "PRESS A TO CALIBRATE" :
                         motion ? "MOVEMENT" : presence ? "POSSIBLE PRESENCE" : "NO MOTION";
    uint32_t color = !fresh || !ready ? 0x728B85 : motion ? 0xFFB148 : presence ? 0x8FE6C1 : 0x38CC83;
    k10.canvas->canvasRectangle(0, 0, 240, 320, 0x030D0A, 0x030D0A, true);
    drawScreenText("ESPRESSIF CSI", 12, 7, 0x48E89B);
    drawScreenText(status, 12, 33, color);
    constexpr int cx = 120, cy = 163;
    for (int r : {
                30, 60, 90
            }) k10.canvas->canvasCircle(cx, cy, r, 0x164D38, 0, false);
    line(24, cy, 216, cy, 0x164D38); line(cx, 67, cx, 259, 0x164D38);
    // Sweep is decorative. The full ring is an activity indicator, never a position.
    float angle = (now % 4000) * (2 * PI / 4000.0f);
    for (int i = 0; i < 5; ++i) {
        float a = angle - i * 0.045f;
        line(cx, cy, cx + 89 * cosf(a), cy + 89 * sinf(a), i ? 0x123D2A : color);
    }
    if (fresh && ready && (motion || presence)) {
        int radius = 48 + static_cast<int>(activity * 30);
        k10.canvas->canvasCircle(cx, cy, radius, color, 0, false);
    }
    if (pendingTraining) {
        drawScreenText(String(10 - min(10u, static_cast<unsigned>((now - leaveAt) / 1000))), 110, 151, 0xFFE080);
    }
    if (training) {
        unsigned percent = min(99u, static_cast<unsigned>((now - trainAt) * 100 / CALIBRATION_MS));
        drawScreenText(String(percent) + "%", 100, 151, 0xFFE080);
    }
    drawScreenText("POSITION UNKNOWN", 27, 266, 0xB9CAC2);
    drawScreenText(String("A:cal B:") + (sensitivityProfile == 0 ? "NORMAL" : sensitivityProfile == 1 ? "HIGH" : "MAX"), 12, 293, 0x728B85);
    k10.canvas->updateCanvas();
    static uint32_t lastLog = 0;
    if (now - lastLog >= 1000) {
        lastLog = now;
        Serial.printf("RADAR,%lu,%s,rssi=%d,frames=%lu,reports=%lu,activity=%.3f\n",
                      now, status, rawRssi.load(), rawCount.load(), reportCount, activity);
        Serial.printf("DIAG,wifi=%d,ap_rssi=%d,disconnect_reason=%lu,ping_ok=%lu,ping_timeout=%lu,running=%d,training=%d\n",
                      static_cast<int>(WiFi.status()), WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0,
                      disconnectReason.load(), pingReplies.load(), pingTimeouts.load(), running, training);
        Serial.printf("METRICS,wander=%.8f,jitter=%.8f,median=%.8f,motion_threshold=%.8f,sensitivity=%.2f,hits=%u,presence_valid=%d,dropped=%lu\n",
                      decision.wanderAverage, decision.lastJitter, decision.jitterMedian, decision.movementThreshold,
                      decision.movementSensitivity, decision.movementHits, decision.presenceKnown, droppedReports.load());
        if (error.length()) {
            Serial.println("ERROR," + error);
        }
    }
}
} // namespace

void setup()
{
    Serial.begin(115200);
    esp_log_set_vprintf(discardNativeLog);
    k10.begin(); k10.initScreen(SCREEN_DIR); k10.creatCanvas();
    results = xQueueCreate(16, sizeof(Result));
    if (!results) {
        error = "Result queue allocation failed";
        Serial.println(error);
        return;
    }
    configValid = strlen(WIFI_SSID) > 0 && strlen(WIFI_SSID) <= 32 && strlen(WIFI_PASSWORD) <= 63;
    if (!configValid) {
        error = "Invalid Wi-Fi config";
        Serial.println(error);
        return;
    }
    WiFi.persistent(false); WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true);
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            disconnectReason.store(info.wifi_sta_disconnected.reason);
        }
        if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
            disconnectReason.store(0);
        }
    });
    Serial.println("BUILD: K10 esp-radar 0.2.0 v4, original decision rules, HIGH sensitivity");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void loop()
{
    uint32_t now = millis();
    if (results && configValid) {
        if (WiFi.status() != WL_CONNECTED) {
            if (running) {
                stopCapture();
            }
        } else if (!running && now - lastAttempt >= 5000) {
            lastAttempt = now;
            if (!startCapture()) {
                stopCapture();
            }
        }
        static bool previousButton = false, debounced = false;
        bool pressed = k10.buttonA && k10.buttonA->isPressed();
        if (pressed != previousButton) {
            previousButton = pressed;
            buttonChanged = now;
        }
        if (pressed != debounced && now - buttonChanged >= 40) {
            debounced = pressed;
            if (pressed && running && !startTraining()) {
                stopCapture();
            }
        }
        static bool previousB = false;
        static uint32_t lastB = 0;
        bool pressedB = k10.buttonB && k10.buttonB->isPressed();
        if (pressedB && !previousB && now - lastB >= 250) {
            lastB = now; sensitivityProfile = (sensitivityProfile + 1) % 3;
            decision.movementSensitivity = sensitivityProfile == 0 ? 0.20f : sensitivityProfile == 1 ? 0.35f : 0.50f;
            decision.clear(); motion = presence = false;
        }
        previousB = pressedB;
        if (pendingTraining && millis() - leaveAt >= 10000 && !beginTraining()) {
            stopCapture();
        }
        Result result{};
        for (unsigned i = 0; i < 16 && xQueueReceive(results, &result, 0) == pdTRUE; ++i) {
            process(result);
        }
        uint32_t received = rawAt.load();
        now = millis();
        bool stale = (reportAt && now - reportAt >= DATA_TIMEOUT_MS) ||
                     (received && now - received >= DATA_TIMEOUT_MS);
        if (ready && stale) {
            // Don't keep an occupied/empty claim across loss of measurements.
            clearDetection();
            xQueueReset(results);
        }
        if (training && now - trainAt >= CALIBRATION_MS && reportCount >= 30 &&
                rawCount.load() - trainCount >= 500 && reportAt && now - reportAt < DATA_TIMEOUT_MS) {
            finishTraining();
        }
        if (training && now - trainAt > 60000) {
            cancelTraining(); training = false; clearDetection();
            error = "Not enough CSI; check router and press A"; Serial.println(error);
        }
    }
    if (now - lastDraw >= 100) {
        lastDraw = now;
        draw();
    }
    delay(5);
}
