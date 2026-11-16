# AGENTS.md — dashboard_firmware

## Purpose and scope

This repository contains the firmware for the **ESP32-C3 Super Mini** dashboard used alongside the `lexcibien/h_shifter_firmware` project (Waveshare RP2040 Zero).

The ESP32-C3 is responsible for:
- Driving the **GC9A01 240×240 round TFT display** over SPI.
- Displaying game telemetry and shifter status.
- Providing screen selection/navigation and dashboard settings.
- Communicating bidirectionally with the RP2040 over a dedicated hardware UART.
- Eventually serving a lightweight local web UI over Wi-Fi.

The **RP2040 owns the physical shifter, USB HID, and SSD1306 I²C OLED**. Do not move those responsibilities into this repository. A disconnected, restarting, or unresponsive dashboard must **never prevent the shifter from working**.

These instructions apply to all files in this repository. Prefer small, testable changes; retain functioning behavior during migrations.

## Current repository state (legacy)

Before changing the project, inspect the existing files. As of the initial migration plan:
- `platformio.ini`: Arduino/PlatformIO environments. `test_tft` extends `rpipico2`, **not** the ESP32-C3 environment.
- `src/tests/main_tft.cpp`: GC9A01 demo using `TFT_eSPI`, full-screen `TFT_eSprite`, `gauge1`…`gauge6`, `Orbitron_Medium_28`, a triangular needle, and serial `VALUE=` / `MENU=` commands.
- `src/main.cpp`: U8g2/SSD1306 Arduino demo; **not the target display implementation** for ESP32-C3.
- `README.md`: primarily documents the old RP2350 GC9A01 wiring.

Treat these files as **migration references**, not as the final architecture. Preserve gauge artwork, scaling, fonts, visual layout, and working behavior where practical. Never claim the ESP-IDF migration is complete while the project still builds only through PlatformIO.

## Target toolchain and dependencies

- Primary target: **ESP32-C3**, with **ESP-IDF** and CMake (`idf.py`).
- Prefer the native ESP-IDF stack (`esp_lcd`, SPI master, UART driver, NVS, `esp_http_server`, FreeRTOS).
- For GC9A01 use an ESP-IDF-compatible panel driver, such as the Espressif `esp_lcd_gc9a01` component, with a version pinned in `idf_component.yml` and tested against the chosen ESP-IDF version.
- Add **LVGL only if justified** by actual screen/navigation complexity and measured memory consumption. `esp_lcd` itself is a panel/transfer API, **not** a replacement for `TFT_eSPI`'s drawing primitives.
- `TFT_eSPI` may remain for legacy demos or as a temporary migration option. Do not introduce Arduino-as-component into the new production design without documenting the reason and tradeoffs.
- Keep `sdkconfig.defaults` for intentionally shared defaults; do not commit machine-specific configuration or credentials.
- Do not guess framework APIs or component versions. Verify against installed ESP-IDF headers and pinned dependency versions before changing code.

## Intended directory structure

Build the structure incrementally; do **not** create empty modules solely to match this diagram.

```text
dashboard_firmware/
├── AGENTS.md
├── README.md
├── CMakeLists.txt
├── sdkconfig.defaults
├── main/
│   ├── CMakeLists.txt
│   └── app_main.cpp
├── components/
│   ├── display/
│   │   ├── include/
│   │   └── ...                 # GC9A01 / esp_lcd, orientation, backlight
│   ├── dashboard/
│   │   ├── include/
│   │   └── ...                 # Screens, gauges, rendering, navigation
│   ├── telemetry/
│   │   ├── include/
│   │   └── ...                 # Typed state, validation, freshness
│   ├── communication/
│   │   ├── include/
│   │   └── ...                 # UART transport and framing
│   ├── settings/
│   │   ├── include/
│   │   └── ...                 # NVS and config schema
│   └── web_server/
│       ├── include/
│       └── ...                 # HTTP routes and static assets
├── web/                        # Source HTML/CSS/JS, if needed
├── assets/                     # Gauge backgrounds, fonts, conversion tools
├── tests/                      # Host-side pure-logic/protocol tests
└── legacy/                     # Optional archived Arduino experiments
```

Use ordinary ESP-IDF component `CMakeLists.txt` files and `idf_component.yml` manifests when dependencies are needed. Prefer **C++ for application logic** and C-compatible interfaces only at stable module boundaries. Keep the entry point thin: initialization, dependency wiring, and task startup rather than application logic.

## Module boundaries

### Display and rendering

- Target GC9A01, 240×240, RGB565 over SPI; configure SPI pins, CS, DC, reset and optional backlight through one board configuration.
- Do not hard-code an unverified ESP32-C3 Super Mini pinout. Check **strapping pins**, native USB Serial/JTAG pins, and board wiring before allocating GPIOs.
- Start with conservative SPI settings; increase clock only after a stable hardware test.
- Avoid unnecessary full-screen copies and redraws. A 240×240 RGB565 framebuffer requires **115,200 bytes (112.5 KiB)**. Prefer partial/line buffers when feasible.
- A driver-allocated transfer buffer must remain valid until the associated transfer has completed; respect the driver callback/synchronization contract.
- Separate display transport from drawing and from telemetry state. The renderer must not parse UART packets or access HTTP requests.
- Preserve gauge artwork and the existing needle behavior when migrating. Validate pixel format, byte order, rotation, alignment and font conversion.
- Never write outside framebuffers or lookup tables. Normalize needle indices and clamp telemetry values.
- Use bounded refresh rates (initially aim for **20–30 FPS**, subject to measurement), avoid busy loops, and render at a lower rate when content is unchanged.
- Only one task should own display rendering unless synchronization is explicitly designed.

### UART protocol and telemetry

- Use a dedicated **hardware UART**, initially **115200, 8N1, 3.3 V TTL**, with TX↔RX and a shared GND. Pin assignment must be explicit and verified.
- Use nonblocking/event-driven reception with bounded queues and timeouts.
- Keep transport, framing, message serialization, and typed application state separate.
- Define a **versioned protocol shared with `h_shifter_firmware`**. Prefer a compact framed binary format with message type, length, sequence, and integrity check (for example COBS framing plus CRC-16). These are *design targets*, not an already implemented protocol.
- Define message types for shifter state, game telemetry, dashboard settings, status/heartbeat, and error reporting.
- Specify byte order, field widths, units, value ranges, maximum payload size, version negotiation, and the meaning of missing values in the protocol documentation.
- Use the same shared protocol definitions in both repositories, pinned to compatible versions. Do not silently change the wire format.
- Keep text commands `VALUE=` and `MENU=` only as optional development compatibility; do not make them the production wire protocol.
- Validate every inbound length, enum, CRC, range, and index before use. Reject malformed packets without blocking rendering.
- Make stale/offline state visible. A lost UART link must not hang the dashboard or create unsafe commands for the shifter.
- Treat telemetry from the PC as distinct from the **physical gear selection** and from the **gear reported by the game**.
- Do not assume the ESP32-C3 directly reads ETS2 telemetry. The initial proposed path is **PC → RP2040 USB CDC → UART → ESP32-C3**; alternative Wi-Fi ingress requires an explicit design change.

### Web and settings (planned)

- Keep the website lightweight: static HTML/CSS/JS served using `esp_http_server`, with REST endpoints and WebSocket only when actually needed.
- Persist user-configurable settings in NVS with schema versioning, validation, and safe defaults.
- Do not block the display/UART tasks during HTTP requests, Wi-Fi reconnects, or NVS writes.
- Restrict input sizes, validate API payloads, avoid unbounded dynamic allocations, and never expose secrets in logs or the frontend bundle.
- Do not commit Wi-Fi credentials, tokens, private keys or generated secrets.
- Changes initiated from the web must pass through validated application commands; never have an HTTP handler modify display buffers or UART driver internals directly.
- The dashboard and shifter should continue to function without Wi-Fi.

## Concurrency and performance

- ESP32-C3 is **single-core**; Wi-Fi, rendering, UART and web handling share CPU and RAM.
- Prefer event-driven FreeRTOS tasks and queues over polling without delay.
- Keep ownership of mutable state explicit. Use snapshots, message passing, or suitable synchronization primitives.
- Avoid large stack allocations and uncontrolled heap use; check errors from all allocations and ESP-IDF APIs.
- Measure `heap_caps_get_free_size()`, minimum free heap, task stack high-water marks, display throughput, UART error counts, and Wi-Fi behavior on hardware.
- Do not claim stable performance without measuring under realistic load (display refresh + UART telemetry + active Wi-Fi/web client).

## Coding conventions

- Follow the existing code style where consistent, but prefer clear modern C++ with minimal global mutable state.
- Keep functions small and modules cohesive. Use descriptive names and explicit units (`rpm`, `speed_kmh`, `timestamp_ms`).
- Prefer `constexpr`, fixed-size arrays and strongly typed enums for protocol and display parameters.
- Avoid `String`, Arduino `setup()/loop()`, `delay()`, and Arduino GPIO helpers in newly migrated ESP-IDF production code.
- Use `ESP_LOGx` with module tags; keep high-frequency logs out of the rendering hot path.
- Check the return codes of `esp_err_t` operations and provide useful diagnostics.
- Do not silently discard code or assets. Record intentional behavior changes in the README or relevant migration notes.
- All comments and developer documentation may be written in **Brazilian Portuguese**; keep identifiers and protocol field names consistently in English.

## Build, tests and validation

For the **new ESP-IDF application**:

```bash
idf.py set-target esp32c3
idf.py build
idf.py -p <PORT> flash monitor
```

These commands become applicable **after** the root ESP-IDF CMake project is created; they are not assertions that the current repository already supports them.

At minimum, verify:
1. Builds cleanly with the selected ESP-IDF version.
2. GC9A01 initializes, displays backgrounds correctly, and updates the needle/text without corruption.
3. Screen selection never reads past the six-gauge table (`0..5` in the legacy demo).
4. Angles and array indices remain valid at minimum, maximum and out-of-range input.
5. UART recovers from truncated packets, bad CRC, lost communication and RP2040 reboot.
6. Dashboard stays responsive while Wi-Fi is reconnecting and a web client is active.
7. Settings validate, persist and recover safely from invalid or missing NVS data.
8. No unintended changes to the **separate** RP2040 HID firmware.

Host tests should cover protocol framing/CRC, parsing, range conversion, and angle math. If a hardware test or build cannot be run, say so explicitly instead of reporting it as passed.

## Known migration hazards

- Legacy `MENU=6` is accepted while the gauge arrays have **six entries indexed 0..5**: fix this before reusing the parser.
- Legacy needle lookup expressions such as `x2[a1 - 2]` may produce invalid indices: normalize angles/indices and test boundaries.
- Legacy `loop()` redraws the whole frame without pacing: introduce a defined update cadence.
- Legacy gauges and `Orbitron_Medium_28` rely on TFT_eSPI-specific image/font conventions: verify conversions visually.
- `TFT_eSprite` full-frame allocation may compete with Wi-Fi and HTTP memory; measure before choosing a rendering backend.
- The `src/main.cpp` SSD1306 demo belongs to the legacy experiments. The **production OLED lives on the RP2040**, not here.

## Rules for coding agents

Before editing:
1. Read this file, `README.md`, `platformio.ini`, and the files affected by the requested change.
2. Distinguish **existing implementation** from **planned architecture**. Never describe unimplemented features as present.
3. State any hardware assumptions (GPIOs, display variant, wiring) clearly; ask only when those details are essential and cannot be inferred.

When editing:
1. Prefer narrowly scoped, reviewable commits/patches.
2. Do not rewrite the RP2040 firmware from this repository.
3. Do not remove legacy functional test assets before an equivalent ESP-IDF implementation works.
4. Keep the UART protocol compatible with both devices or explicitly version the breaking change.
5. Update documentation and build files together with code.

After editing:
1. Run the available build/tests and report exactly what succeeded or was not run.
2. Summarize changed files, behavior, hardware assumptions and outstanding risks.
3. Never invent benchmark results, successful flashes, wiring verification, or hardware tests.
