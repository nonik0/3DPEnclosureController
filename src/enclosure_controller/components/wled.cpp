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
        // log_i("Response: %s", jsonResponse.c_str());
        _wled_client.end();

        JsonDocument jsonDoc;
        DeserializationError error = deserializeJson(jsonDoc, jsonResponse);
        if (error)
        {
            log_e("deserializeJson() failed: %s", error.c_str());
            return false;
        }

        for (int i = 0; i < 16; i++)
        {
            _wled_presets[i] = {-1, ""};
        }

        int presetIndex = 0;
        for (JsonPair preset : jsonDoc.as<JsonObject>())
        {
            int presetId = atoi(preset.key().c_str());
            String presetName = preset.value()["n"].as<String>();
            if (presetName != "null" && presetId < 16)
            {
                _wled_presets[presetIndex] = {presetId, presetName};
                log_i("Preset %d: %s", presetId, presetName.c_str());
            }

            presetIndex++;
        }

        // sort presets by name, with empty names at the end
        std::sort(std::begin(_wled_presets), std::end(_wled_presets), [](const WLEDPreset &a, const WLEDPreset &b)
                  {
            if (a.name == "" && b.name == "")
                return false;
            if (a.name == "")
                return false;
            if (b.name == "")
                return true;
            return a.name < b.name; });

        for (int i = 0; i < 16; i++)
            log_i("Preset: idx[%d] id[%d] name[%s]", i, _wled_presets[i].id, _wled_presets[i].name.c_str());
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
    TaskHandle_t updateTaskHandle = NULL;

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
            brightnessPct = _enc_pos > old_position
                                ? min(brightnessPct + 1, maxBrightnessPct)
                                : max(brightnessPct - 1, minBrightnessPct);

            _wled_brightness = brightnessPct * 2.55;
            old_position = _enc_pos;

            if (updateTaskHandle == NULL)
            {
                xTaskCreate([](void *p)
                            {
                                EnclosureController *self = (EnclosureController *)p;
                                int lastUpdatedBrightness = self->_wled_brightness;

                                while (true) {
                                    if (self->_wled_on && self->_wled_brightness != lastUpdatedBrightness) {
                                        String json = "{\"bri\":" + String(self->_wled_brightness) + "}";
                                        self->_wled_send_command(json);
                                        lastUpdatedBrightness = self->_wled_brightness;
                                        log_i("Updated brightness to %d", lastUpdatedBrightness);
                                    }

                                    vTaskDelay(10);
                                } }, "WLEDBrightnessUpdateTask", 4096, this, 1, &updateTaskHandle);
            }
        }

        if (_check_btn() == SHORT_PRESS)
        {
            String json = _wled_on
                              ? "{\"on\":false}"
                              : "{\"on\":true,\"bri\":" + String(_wled_brightness) + "}";
            if (_wled_send_command(json))
                _wled_on = !_wled_on;
            break;
        }
        else if (_check_btn() == LONG_PRESS)
        {
            break;
        }
    }

    if (updateTaskHandle != NULL)
    {
        vTaskDelete(updateTaskHandle);
    }
}

void EnclosureController::_wled_set_preset()
{
    const int minPresetIndex = 0;
    const int maxPresetIndex = 15;

    _wled_update_presets();

    _canvas->setFont(&fonts::Font0);

    // find current preset index if set
    int curPresetIndex = -1;
    if (_wled_preset > -1)
    {
        for (int i = 0; i < 16; i++)
        {
            if (_wled_presets[i].id == _wled_preset)
            {
                curPresetIndex = i;
                break;
            }
        }
    }
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

        _canvas->setTextSize(3);
        _canvas->setTextColor((uint32_t)0x07430F);
        if (curPresetIndex == -1)
        {
            snprintf(string_buffer, 20, "None");
            _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 55);
        }
        else
        {
            String name = _wled_presets[curPresetIndex].name;
            int firstSpaceIdx = name.indexOf(' ');

            if (firstSpaceIdx > 0) // 2 lines for name, split on first space
            {
                _canvas->drawCenterString(name.substring(0, firstSpaceIdx), _canvas->width() / 2, 55 - _canvas->fontHeight() / 2 - 1);
                _canvas->drawCenterString(name.substring(firstSpaceIdx), _canvas->width() / 2, 55 + _canvas->fontHeight() / 2 + 1);
            }
            else // one line
            {
                snprintf(string_buffer, 20, "%s", _wled_presets[curPresetIndex].name.c_str());
                _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 55);
            }
        }

        _canvas_update();

        if (_check_encoder())
        {
            do
            {
                curPresetIndex = _enc_pos > old_position ? curPresetIndex + 1 : curPresetIndex - 1;
                if (curPresetIndex < minPresetIndex)
                    curPresetIndex = maxPresetIndex;
                if (curPresetIndex > maxPresetIndex)
                    curPresetIndex = minPresetIndex;
            } while (_wled_presets[curPresetIndex].name == "");

            old_position = _enc_pos;
        }

        if (_check_btn() == SHORT_PRESS)
        {
            _wled_preset = _wled_presets[curPresetIndex].id;

            log_i("setting preset to %d", _wled_preset);
            String json = "{\"on\":true,\"ps\":" + String(_wled_preset) + "}";
            _wled_send_command(json);
            break;
        }
        else if (_check_btn() == LONG_PRESS)
        {
            break;
        }
    }
}