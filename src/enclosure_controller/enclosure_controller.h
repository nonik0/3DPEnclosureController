#pragma once
#include <Arduino.h>
#include <Button.h>
#include <ESP32Encoder.h>
#include <I2C_BM8563.h>
#include <LovyanGFX.h>

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <libssh/libssh.h>
#include <libssh_esp32.h>

#define FW_VERISON "v0.1"
#define BUZZ_PIN 3
#define POWER_HOLD_PIN 46

class EnclosureController
{
public:
    /* System */
    inline void _stuck_forever()
    {
        while (1)
        {
            delay(100);
        }
    }
    void _power_on();
    void _power_off();

    /* Display */
    LGFX_Device* _disp = nullptr;
    LGFX_Sprite* _canvas = nullptr;
    inline void _canvas_update() { _canvas->pushSprite(0, 0); }
    void _disp_init();
    void _disp_set_brightness();

    /* Button */
    enum PressType
    {
        NONE,
        SHORT_PRESS,
        LONG_PRESS
    };
    Button _enc_btn = Button(42, 20);
    void _btn_init();
    PressType _check_btn();
    void _wait_next();

    /* Encoder */
    ESP32Encoder _enc;
    int _enc_pos = 0;
    void _enc_init();
    bool _check_encoder(bool playBuzz = true);

    /* Buzzer */
    inline void _tone(unsigned int frequency, unsigned long duration = 0UL) { tone(BUZZ_PIN, frequency, duration); }
    inline void _noTone() { noTone(BUZZ_PIN); }

    /* RTC */
    I2C_BM8563 _rtc;
    void _rtc_init();
    void _rtc_ntp_sync();
    void _rtc_check();

    /* Wifi */
    bool _wifi_inited;
    void _wifi_init();
    bool _wifi_check();

    /* SSH */
    ssh_session _ssh_session;
    bool _ssh_inited;
    void _ssh_init();
    void _ssh_deinit();
    void _ssh_cmd(const char* cmd);

    /* Arkanoid */
    void _arkanoid_start();
    void _arkanoid_setup();
    void _arkanoid_loop();
    void _InitGame(void);        // Initialize game
    void _UpdateGame(void);      // Update game (one frame)
    void _DrawGame(void);        // Draw game (one frame)
    void _UnloadGame(void);      // Unload game
    void _UpdateDrawFrame(void); // Update and Draw (one frame)

    /* 3D Printer */
    void _printer_set_extruder_temp();
    void _printer_set_bed_temp();
    void _printer_set_temperature(int minTemp, int maxTemp, int defaultTemp, const char* name, const char* gcode);

public:
    EnclosureController() {}
    ~EnclosureController() = default;

    void init();
};
