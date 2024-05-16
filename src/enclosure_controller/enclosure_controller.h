/**
 * @file factory_test.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-06-06
 *
 * @copyright Copyright (c) 2023
 *
 */
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
    bool _is_test_mode;

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
    void _get_test_mode();
    bool _check_test_mode();

    /* Display */
    LGFX_Device* _disp;
    LGFX_Sprite* _canvas;
    inline void _canvas_update() { _canvas->pushSprite(0, 0); }
    void _disp_init();
    void _disp_test();
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
    // RotaryEncoder _enc = RotaryEncoder(40, 41, RotaryEncoder::LatchMode::TWO03);
    ESP32Encoder _enc;
    int _enc_pos;
    bool _check_encoder(bool playBuzz = true);
    void _encoder_test();
    void _encoder_test_new();
    void _encoder_test_user();

    /* Buzzer */
    inline void _tone(unsigned int frequency, unsigned long duration = 0UL) { tone(BUZZ_PIN, frequency, duration); }
    inline void _noTone() { noTone(BUZZ_PIN); }

    /* RTC */
    I2C_BM8563 _rtc;
    void _rtc_init();
    void _rtc_test();
    void _rtc_wakeup_test();
    void _rtc_wakeup_test_user();

    /* Wifi */
    bool _wifi_inited;
    void _wifi_init();
    void _wifi_check();

    /* SSH */
    ssh_session _ssh_session;
    bool _ssh_inited;
    void _ssh_init();
    void _ssh_cmd(const char* cmd);

    /* BLE */
    bool _is_ble_inited;
    void _ble_test();

    /* IO */
    void _io_test();
    void _io_test_user();

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

public:
    EnclosureController() : _is_test_mode(false), _disp(nullptr), _canvas(nullptr), _enc_pos(0), _is_ble_inited(false) {}
    ~EnclosureController() = default;

    void init();
};