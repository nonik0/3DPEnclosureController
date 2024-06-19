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

#include <M5UnitENV.h>

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
    LGFX_Device *_disp = nullptr;
    LGFX_Sprite *_canvas = nullptr;
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

    /* Env IV Sensor */
    SHT4X _sht4Sensor;
    BMP280 _bmpSensor;
    float _temp = 0.0;
    float _hum = 0.0;
    float _press = 0.0;
    bool _env_inited = false;
    void _env_init();
    void _env_update();

    /* Wifi */
    bool _wifi_inited = false;
    void _wifi_init();
    bool _wifi_check();

    /* SSH */
    ssh_session _ssh_session;
    bool _ssh_inited = false;
    void _ssh_init();
    void _ssh_deinit();
    void _ssh_cmd(const char *cmd);

    /* Arkanoid */
    void _arkanoid_start();
    void _arkanoid_setup();
    void _arkanoid_loop();
    void _InitGame(void);        // Initialize game
    void _UpdateGame(void);      // Update game (one frame)
    void _DrawGame(void);        // Draw game (one frame)
    void _UnloadGame(void);      // Unload game
    void _UpdateDrawFrame(void); // Update and Draw (one frame)

    /* WLED (enclosure lighting) */
    volatile bool _wled_on = false;
    volatile int _wled_brightness = 0;
    // uint32_t _wled_color = 0;
    // uint8_t _wled_effect = 0;
    int _wled_preset = -1;
    String _wled_preset_names[16];
    HTTPClient _wled_client;
    bool _wled_update_state(String json = "");
    bool _wled_update_presets();
    bool _wled_send_command(String json);
    void _wled_set_brightness();
    void _wled_set_preset();
    // void _wled_set_color();
    // void _wled_set_effect();

    /* 3D Printer - Prusalink */
    struct PrinterStatus
    {
        String state; // [ IDLE, BUSY, PRINTING, PAUSED, FINISHED, STOPPED, ERROR, ATTENTION, READY ]
        float temp_nozzle;
        float temp_bed;
        float speed;
        float target_nozzle;
        float target_bed;
    };
    struct PrinterJob
    {
        int id;
        int progress;
        int time_remaining;
    };
    PrinterStatus _printer_status = {"UNKNOWN", 0, 0, 0, 0, 0};
    PrinterJob _printer_job = {0, 0, 0};
    String _printer_api_authreq = "";
    uint _printer_api_nonce;
    String _printer_send_api_request(String httpMethod, String requestUri);
    void _printer_update_status();
    void _printer_set_extruder_temp();
    void _printer_set_bed_temp();
    void _printer_set_temperature(int minTemp, int maxTemp, int defaultTemp, const char *name, const char *gcode);
    void _printer_manage_job();

public:
    EnclosureController() {}
    ~EnclosureController() = default;

    void init();
};
