/**
 * @file disp_test.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-06-06
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "LGFX_DinMeter.hpp"
#include "../enclosure_controller.h"

void EnclosureController::_disp_init()
{
    _disp = new LGFX_DinMeter;
    _disp->init();
    _disp->setRotation(3);

    _canvas = new LGFX_Sprite(_disp);
    _canvas->createSprite(_disp->width(), _disp->height());
}

void EnclosureController::_disp_set_brightness()
{
    printf("set brightness\n");

    _canvas->setFont(&fonts::Font0);

    int brightness = _disp->getBrightness();
    long old_position = _enc_pos;
    char string_buffer[20];

    _enc_pos = 0;
    _enc.setPosition(_enc_pos);

    while (1)
    {
        _canvas->fillScreen((uint32_t)0xB8DBD9);

        _canvas->fillRect(0, 0, 240, 25, (uint32_t)0x385B59);
        _canvas->setTextSize(2);
        _canvas->setTextColor((uint32_t)0xB8DBD9);
        snprintf(string_buffer, 20, "Set Brightness");
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 5);

        _canvas->setTextSize(5);
        _canvas->setTextColor((uint32_t)0x385B59);
        snprintf(string_buffer, 20, "%d", brightness);
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 55);

        _canvas_update();

        if (_check_encoder())
        {
            if (_enc_pos > old_position)
            {
                brightness += 5;

                printf("add\n");
            }
            else
            {
                brightness -= 5;

                printf("min\n");
            }

            if (brightness > 255)
            {

                brightness = 255;

                printf("hit top\n");
            }
            else if (brightness < 0)
            {
                brightness = 0;

                printf("hit bottom\n");
            }

            old_position = _enc_pos;
            _disp->setBrightness(brightness);
        }

        if (_check_btn())
        {
            break;
        }
    }

    printf("quit set brightness\n");
}

// called by input functions to reset the screen timeout
void EnclosureController::_disp_timeout_reset()
{
    _disp_timeout = millis() + 3 * 60 * 1000; // 3 minute screen timeout
}

bool EnclosureController::_disp_blank_if_timeout()
{
    if (millis() > _disp_timeout)
    {
        if (_disp_saved_brightness == 0)
        {
            _disp_saved_brightness = _disp->getBrightness();
            _disp->setBrightness(0);
            log_i("Screen timeout, blanking display and saving brightness %d", _disp_saved_brightness);
        }

        return true;
    }
    else if (_disp_saved_brightness > 0)
    {
        log_i("Restoring brightness to %d", _disp_saved_brightness);
        _disp->setBrightness(_disp_saved_brightness);
        _disp_saved_brightness = 0;
    }

    return false;
}