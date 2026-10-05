# Y2K Bluetooth MP3 Player 🎵

> A modern, dual-microcontroller portable music player inspired by the aesthetic of classic Y2K-era gadgets — rebuilt with 2024 hardware and a sleek LVGL-powered touch UI.

## 1. Project Overview & Architecture
The Y2K Bluetooth MP3 Player is a modern recreation of classic portable media players, utilizing a dual-microcontroller architecture to separate heavy UI rendering from real-time audio processing and wireless communication. 

The system leverages two distinct microcontrollers:
*   **RP2040 (Raspberry Pi Pico / mbed core):** Dedicated to driving a 320x240 ILI9341 TFT Touch Display. It runs the LVGL (Light and Versatile Graphics Library) framework for fluid, modern UI rendering.
*   **ESP32:** Acts as the "Audio Backend". It is responsible for reading media files from an SD Card, handling MP3 decoding, and managing Bluetooth Audio (A2DP) connections.

This separation of concerns ensures that the UI remains responsive (running at high framerates on the RP2040) while the ESP32 handles the memory-intensive and timing-critical tasks of audio streaming and decoding.

## 2. Hardware Components & Wiring
### Microcontrollers
*   **RP2040 (Master UI Controller)**
*   **ESP32 (Slave Audio/BT Controller)**

### Peripherals
*   **Display:** ILI9341 320x240 TFT LCD with Touch capability.
*   **Storage:** MicroSD Card module (connected to ESP32 via SPI).
*   **Audio Output:** I2S DAC or internal DAC on the ESP32.

### RP2040 ↔ ESP32 UART Bridge
The two boards communicate via high-speed UART (115200 baud). A common ground between both boards is critical for signal integrity.

| RP2040 Pin | ESP32 Pin | Function |
|------------|-----------|----------|
| GP0 (TX) | Pin 16 (RX) | RP2040 → ESP32 commands |
| GP1 (RX) | Pin 17 (TX) | ESP32 → RP2040 state updates |
| GND | GND | Common ground (**required!**) |

> ⚠️ **Critical:** If GND is not shared between both boards, the UART signal will be corrupted and all inter-chip communication will fail silently.

## 3. Software Stack - RP2040 (UI Frontend)
The RP2040 runs an Arduino sketch (`LVGL_Arduino_Lovyan.ino`) utilizing the `LovyanGFX` library for low-level display drivers and `LVGL v9` for high-level UI widgets.

### Screens & Views
The UI is divided into several distinct screens, managed by LVGL:
1.  **Now Playing (Screen 1):** Displays current track title, artist, playback progress, and album art (wallpaper). Contains play/pause, next, previous, shuffle, and repeat controls.
2.  **Music Library (Screen 2):** A scrollable list of MP3 files found on the SD card. Supports filtering (All Songs, Favorites, Folders). Handled by `create_screen_library()` and dynamically updated by `rebuild_track_ui()`.
3.  **Bluetooth Audio (Screen 3):** Allows scanning and pairing with nearby Bluetooth audio devices/sinks.
4.  **Equalizer & Sound (Screen 4):** Audio settings and EQ sliders.
5.  **Queue & Playlists (Screen 5):** Upcoming tracks.
6.  **System Settings (Screen 6):** Device configuration and color themes.

## 4. Software Stack - ESP32 (Audio Backend)
The ESP32 runs `ESP32_Audio_Backend.ino`. Its primary responsibilities are:
1.  **SD Card Management:** Mounting the FAT32/exFAT filesystem and scanning for `.mp3` files on boot.
2.  **Audio Playback:** Decoding MP3s and pushing the PCM data to the DAC.
3.  **Bluetooth:** Broadcasting as an audio source or connecting to sinks.
4.  **Command Parsing:** Listening to `Serial2` for commands from the RP2040 (e.g., `CMD:PLAY`, `CMD:NEXT`).

## 5. Serial Protocol (UART Bridge)
A custom text-based protocol is used over the UART connection.

### ESP32 -> RP2040 (State Updates)
*   `TRACKS:<count>` - Informs the UI how many tracks were found.
*   `TRACK:<index>:<filename>` - Sends individual track metadata.
*   `TITLE:<title>` - Updates the "Now Playing" title.
*   `ARTIST:<artist>` - Updates the "Now Playing" artist.
*   `STATUS:PLAYING` / `STATUS:PAUSED` - Syncs the play/pause button state.
*   `BT_CONNECTED:<device_name>` - Notifies UI of successful BT pairing.

### RP2040 -> ESP32 (Control Commands)
*   `CMD:PLAY` - Start or resume playback.
*   `CMD:PAUSE` - Pause playback.
*   `CMD:NEXT` - Skip to the next track.
*   `CMD:PREV` - Go to the previous track.
*   `CMD:PLAYTRACK:<index>` - Play a specific track from the library.

## 6. Known Issues & Recent Fixes
### 1. Undefined Reference: `rebuild_track_ui()`
*   **Issue:** The RP2040 code failed to compile because the function `rebuild_track_ui()` was declared and called but never defined.
*   **Resolution:** Implemented the function to properly call `lv_obj_clean(lib_track_list)` and regenerate the track list rows dynamically.

### 2. UART Baud Rate Mismatch (Garbled Data)
*   **Issue:** The RP2040 was sending commands, but the ESP32 logged garbage characters and failed to load the track list.
*   **Resolution:** Aligned both boards to 115200 baud and explicitly defined the RX (16) and TX (17) pins to ensure hardware alignment.

## 7. Future Roadmap
*   **Album Art Extraction:** Parse ID3 tags on the ESP32 to extract embedded cover art.
*   **OTA Updates:** Enable Over-The-Air firmware updates for both the ESP32 and RP2040 via WiFi.
*   **Extended Format Support:** Add support for FLAC, WAV, and AAC decoding on the ESP32.
