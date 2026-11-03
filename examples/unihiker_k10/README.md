# UNIHIKER K10 CSI motion sensing

Standalone PlatformIO/Arduino example for DFRobot UNIHIKER K10 (ESP32-S3),
using its built-in display, buttons and a 2.4 GHz access point. It supports
the board vendor's legacy ESP-IDF 4.4.4 SDK without changing the shared
ESP-CSI examples or the vendor binary algorithm.

This example intentionally pins **esp-radar 0.2.0**. It does not use the
current esp-radar API and is not an alternative configuration of the
modern `wifi_sensing_demo`.

## Hardware and build

- UNIHIKER K10 and a USB data cable.
- A stationary 2.4 GHz router or phone hotspot that answers ICMP echo requests.
- PlatformIO Core and Python 3.

The platform and Arduino framework commits are pinned in `platformio.ini`.
The CSI ABI adapter accepts only ESP32-S3 / ESP-IDF 4.4.4 and checks structure
offsets at compile time. Do not remove these checks when upgrading the SDK.

From this directory:

```sh
python scripts/fetch_esp_radar.py
```

The helper downloads the official component archive, verifies its SHA-256,
and installs only the ESP32-S3 archive, header, manifest and license under
ignored `lib/esp-radar/`. A previously downloaded official archive can be used:

```sh
python scripts/fetch_esp_radar.py --archive /path/to/esp-radar-0.2.0.zip
```

Copy `include/board_config.example.h` to **ignored** `include/board_config.h`
and set your SSID and password locally. Then:

```sh
pio run
pio run --target upload --upload-port COM16
pio device monitor --port COM16 --baud 115200
```

Replace `COM16` with the actual device port. Close any existing serial
monitor or other program holding that port before uploading. Uploading
replaces the device application; `Model=None` avoids uploading the vendor
speech model. No full-flash erase is needed.

Do not commit `board_config.h`, build outputs or flash dumps: compiled
firmware embeds the configured Wi-Fi credentials.

## Display and controls

The display uses a circular activity view. **The sweep is an animation and
the ring radius is signal activity, not object angle or distance.** No
object coordinates, silhouettes or person identities are produced.

- **A:** allow 10 seconds to leave the sensing area, then collect at least
  15 seconds of calibration data in an empty, still room. Keep both the
  board and access point stationary.
- **B:** cycle motion sensitivity: `NORMAL` (0.20, original console example),
  `HIGH` (0.35, startup default), `MAX` (0.50). Higher sensitivity can also
  increase false detections. Changing B does not require recalibration.
- `MOVEMENT`: the motion rule is active.
- `NO MOTION`: no motion was detected; this does **not** mean an empty room.
- `POSSIBLE PRESENCE`: available only with a valid positive native wander
  calibration threshold.
- `WAITING FOR CSI`: measurements or algorithm reports are stale/unavailable.

Wi-Fi reconnection schedules calibration again. A measurement gap invalidates
the displayed decision and requires A before resuming classification.

## Detection rules and compatibility adaptations

`original_decision.h` follows the `wifi_radar_cb` rules shipped with
esp-radar 0.2.0 `console_test`:

- 25-report history, native `trimmean(..., 0.5)` for wander and native
  `median()` for jitter;
- motion if at least **two of the latest five** reports exceed the fixed
  threshold, or the original adaptive median rule;
- original adaptive rule: `jitter * sensitivity > jitter_median` and
  `jitter > 0.0002`;
- presence sensitivity 0.15, original RF gain compensation, LLTF/shift=4,
  10 ms ping target interval and original task priorities.

Only filled history slots are used during warm-up. Results are copied through
a bounded 16-report queue rather than displaying/logging from Wi-Fi callbacks.
Non-finite reports are rejected and queue losses are counted.

The K10 driver's `wifi_csi_info_t` has no destination MAC and places `buf`
at offset 56. The 0.2.0 ESP32-S3 binary expects a destination MAC and `buf`
at offset 64. `radar_csi_compat.cpp` wraps CSI callback registration and
translates each argument to that binary layout, preserving the payload,
source MAC, radio metadata and invalid-first-word flag. Destination filtering
is unavailable; source filtering remains restricted to the connected AP.
The archive itself is unmodified. This adapter is specific to this archive/SDK
combination, not a general-purpose ESP-IDF compatibility layer.

The vendor SDK also injects esp-insights log hooks. In hardware testing,
calibration crashed in the formatted logging path. `log_compat.py` removes
those two log hooks, and the application disables native formatted log output.
Explicit `RADAR`, `DIAG`, `METRICS`, `BASELINE` and `ERROR` serial messages remain.

If native calibration returns an invalid presence threshold, this example
continues with **motion only**, without inventing an occupancy threshold.
An invalid motion threshold falls back to the original console default
0.0003; the adaptive branch remains enabled. See `presence_valid` in the
serial diagnostics. This example does not claim reliable stationary-person
detection on the legacy SDK.

The display helper uses the SDK's built-in LVGL font. The hardware prototype
used a separate bitmap font; the sensing code and decision rules are the same.

## Validation and limits

The contributor reports successful practical motion detection on K10.
Recorded prototype validation observed 5,331 CSI frames and 171 post-training
reports by 57.67 seconds after boot, zero queue drops, no panic/reboot during
that observation, and transitions between `MOVEMENT` and `NO MOTION`.
Native wander calibration returned NaN, so that run used motion-only mode.
This is not an accuracy, range, through-wall or false-positive benchmark.

`tests/test_original_decision.cpp` tests the actual decision class with host
statistics adapters: adaptive detection below the fixed threshold, 2/5 voting,
history wraparound, invalid inputs/calibration, sensitivity and warm-up.
The firmware links the statistics functions from the Espressif archive.

For a host C++ compiler, for example:

```sh
c++ -std=c++14 -Iinclude tests/test_original_decision.cpp -o /tmp/test_original_decision
/tmp/test_original_decision
```

## Dependency provenance and license

Official component: <https://components.espressif.com/components/espressif/esp-radar/versions/0.2.0>

Official archive SHA-256:
`97720566ad18a8d51c1af32fb06fb69490bf1cb137a97b68480c17137401c015`.

New example code is Apache-2.0. The downloaded Espressif component retains its
own Apache-2.0 notice and license. No vendor binary library, device credentials,
compiled application, private capture or custom font is checked into this example.
