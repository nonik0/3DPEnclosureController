#include <ArduinoJson.h>
// #include "LGFX_DinMeter.hpp"
#include "../enclosure_controller.h"
#include "../secrets.h"

bool EnclosureController::_wled_update_state(String jsonResponse)
{
    if (jsonResponse == "")
    {
        static String requestUrl = String("http://" + String(WLED_IP) + "/json/state");
        _wled_client.begin(requestUrl);
        int httpResponseCode = _wled_client.GET();
        if (httpResponseCode > 0)
        {
            jsonResponse = _wled_client.getString();
            log_i("HTTP Response code: %d", httpResponseCode);
            log_i("Response: %s", jsonResponse.c_str());
            _wled_client.end();
        }
        else
        {
            log_e("HTTP Error: %d", httpResponseCode);
            _wled_client.end();
            return false;
        }
    }

    if (jsonResponse == "")
    {
        log_e("No response from WLED");
        return false;
    }

    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, jsonResponse);
    if (error)
    {
        log_e("deserializeJson() failed: %s", error.c_str());
        return false;
    }

    _wled_on = jsonDoc["on"];
    _wled_brightness = jsonDoc["bri"];
    _wled_preset = jsonDoc["ps"].as<int>();

    log_i("WLED state: on=%d, bri=%d, ps=%d", _wled_on, _wled_brightness, _wled_preset);
    return true;
}

bool EnclosureController::_wled_update_presets()
{
    static String requestUrl = String("http://" + String(WLED_IP) + "/edit?edit=/presets.json");
    _wled_client.begin(requestUrl);
    int httpResponseCode = _wled_client.GET();
    if (httpResponseCode > 0)
    {
        String jsonResponse = _wled_client.getString();
        log_i("HTTP Response code: %d", httpResponseCode);
        log_i("Response: %s", jsonResponse.c_str());
        _wled_client.end();

        JsonDocument jsonDoc;
        DeserializationError error = deserializeJson(jsonDoc, jsonResponse);
        if (error)
        {
            log_e("deserializeJson() failed: %s", error.c_str());
            return false;
        }

        for (JsonPair preset : jsonDoc.as<JsonObject>())
        {
            int presetId = atoi(preset.key().c_str());
            _wled_preset_names[presetId] = preset.value()["n"].as<String>();
            log_i("Preset %d: %s", presetId, _wled_preset_names[presetId].c_str());
        }
    }
    else
    {
        log_e("HTTP Error: %d", httpResponseCode);
        _wled_client.end();
        return false;
    }

    return true;
}

bool EnclosureController::_wled_send_command(String json)
{
    static String requestUrl = String("http://" + String(WLED_IP) + "/json/state");
    _wled_client.begin(requestUrl);
    _wled_client.addHeader("Content-Type", "application/json");

    log_i("Sending command: %s", json.c_str());
    int httpResponseCode = _wled_client.POST(json);
    if (httpResponseCode > 0)
    {
        String response = _wled_client.getString();
        log_i("HTTP Response code: %d", httpResponseCode);
        log_i("Response: %s", response.c_str());
        _wled_client.end();
        return true;
    }
    else
    {
        log_e("HTTP Error: %d", httpResponseCode);
        _wled_client.end();
        return false;
    }
}

void EnclosureController::_wled_set_brightness()
{
    const int minBrightnessPct = 0;
    const int maxBrightnessPct = 100;

    _wled_update_state();

    _canvas->setFont(&fonts::Font0);

    int brightnessPct = _wled_brightness / 2.55;
    long old_position = 0;
    char string_buffer[20];

    _enc_pos = 0;
    _enc.setPosition(_enc_pos);

    while (1)
    {
        _canvas->fillScreen((uint32_t)0x87C38F);

        _canvas->fillRect(0, 0, 240, 25, (uint32_t)0x07430F);
        _canvas->setTextSize(2);
        _canvas->setTextColor((uint32_t)0x87C38F);
        snprintf(string_buffer, 20, "Set LED Brightness");
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 5);

        _canvas->setTextSize(5);
        _canvas->setTextColor((uint32_t)0x07430F);
        snprintf(string_buffer, 20, "%d%", brightnessPct);
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 55);

        if (!_wled_on)
        {
            _canvas->setTextSize(3);
            _canvas->drawCenterString("OFF", _canvas->width() / 2, 110);
        }

        _canvas_update();

        if (_check_encoder())
        {
            brightnessPct = _enc_pos > old_position ? brightnessPct + 1 : brightnessPct - 1;

            if (brightnessPct > maxBrightnessPct)
                brightnessPct = maxBrightnessPct;
            if (brightnessPct < minBrightnessPct)
                brightnessPct = minBrightnessPct;

            // TODO: rate limiter??
            _wled_brightness = brightnessPct * 2.55;
            String json = "{\"bri\":" + String(_wled_brightness) + "}";
            if (_wled_on)
                _wled_send_command(json);

            old_position = _enc_pos;
        }

        if (_check_btn() == SHORT_PRESS)
        {
            String json = _wled_on
                              ? "{\"on\":false}"
                              : "{\"on\":true,\"bri\":" + String(_wled_brightness) + "}";
            if (_wled_send_command(json))
                _wled_on = !_wled_on;
        }
        else if (_check_btn() == LONG_PRESS)
        {
            break;
        }
    }
}

void EnclosureController::_wled_set_preset()
{
    const int minPresetIndex = 0;
    const int maxPresetIndex = 15;

    _wled_update_presets();

    _canvas->setFont(&fonts::Font0);

    int curPresetIndex = _wled_preset;
    long old_position = 0;
    char string_buffer[20];

    _enc_pos = 0;
    _enc.setPosition(_enc_pos);

    while (1)
    {
        _canvas->fillScreen((uint32_t)0x87C38F);

        _canvas->fillRect(0, 0, 240, 25, (uint32_t)0x07430F);
        _canvas->setTextSize(2);
        _canvas->setTextColor((uint32_t)0x87C38F);
        snprintf(string_buffer, 20, "Set LED Preset");
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 5);

        _canvas->setTextSize(4);
        _canvas->setTextColor((uint32_t)0x07430F);
        if (curPresetIndex == -1)
            snprintf(string_buffer, 20, "None");
        else
            snprintf(string_buffer, 20, "%s", _wled_preset_names[curPresetIndex].c_str());
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 55);

        _canvas_update();

        if (_check_encoder())
        {
            do {
                curPresetIndex = _enc_pos > old_position ? curPresetIndex + 1 : curPresetIndex - 1;
                if (curPresetIndex < minPresetIndex)
                    curPresetIndex = maxPresetIndex;
                if (curPresetIndex > maxPresetIndex)
                    curPresetIndex = minPresetIndex;
            } while(_wled_preset_names[curPresetIndex] == nullptr);

            old_position = _enc_pos;
        }

        if (_check_btn() == SHORT_PRESS)
        {
            _wled_preset = curPresetIndex;
            String json =  "{\"on\":true,\"ps\":" + String(_wled_preset) + "}";
            _wled_send_command(json);
        }
        else if (_check_btn() == LONG_PRESS)
        {
            break;
        }
    }
}