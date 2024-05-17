#include "LGFX_DinMeter.hpp"
#include "../enclosure_controller.h"

// arkanoid orange: 0xF5C396
// arakanoid dark orange: 0x754316

void EnclosureController::_printer_set_extruder_temp()
{
    printf("set extruder temp\n");

    log_i("ssh init");
    _ssh_init();

    _canvas->setFont(&fonts::Font0);

    int temperature = 0; // get current temp from printer
    long old_position = _enc_pos;
    char string_buffer[20];

    _enc_pos = 0;
    _enc.setPosition(_enc_pos);

    while (1)
    {
        _canvas->fillScreen((uint32_t)0xF5C396);

        _canvas->fillRect(0, 0, 240, 25, (uint32_t)0x754316);
        _canvas->setTextSize(2);
        _canvas->setTextColor((uint32_t)0xF5C396);
        snprintf(string_buffer, 20, "Set Extruder Temp");
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 5);

        _canvas->setTextSize(5);
        _canvas->setTextColor((uint32_t)0x754316);
        snprintf(string_buffer, 20, "%dC", temperature);
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 55);

        _canvas_update();

        if (_check_encoder())
        {
            if (_enc_pos > old_position)
            {
                temperature += 5;
                printf("add\n");
            }
            else
            {
                temperature -= 5;
                printf("min\n");
            }

            if (temperature > 255)
            {
                temperature = 255;
                printf("hit top\n");
            }
            else if (temperature < 0)
            {
                temperature = 0;
                printf("hit bottom\n");
            }

            old_position = _enc_pos;
            // send temperature to printer
        }

        if (_check_btn())
        {
            break;
        }
    }

    printf("quit set extruder tenmp\n");
}
  
void EnclosureController::_printer_set_bed_temp()
{
    printf("set bed temp\n");

    _canvas->setFont(&fonts::Font0);

    int temperature = 0; // get current temp from printer
    long old_position = _enc_pos;
    char string_buffer[20];

    _enc_pos = 0;
    _enc.setPosition(_enc_pos);

    while (1)
    {
        _canvas->fillScreen((uint32_t)0xF5C396);

        _canvas->fillRect(0, 0, 240, 25, (uint32_t)0x754316);
        _canvas->setTextSize(2);
        _canvas->setTextColor((uint32_t)0xF5C396);
        snprintf(string_buffer, 20, "Set Bed Temp");
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 5);

        _canvas->setTextSize(5);
        _canvas->setTextColor((uint32_t)0x754316);
        snprintf(string_buffer, 20, "%d°C", temperature);
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 55);

        _canvas_update();

        if (_check_encoder())
        {
            if (_enc_pos > old_position)
            {
                temperature += 5;
                printf("add\n");
            }
            else
            {
                temperature -= 5;
                printf("min\n");
            }

            if (temperature > 255)
            {
                temperature = 255;
                printf("hit top\n");
            }
            else if (temperature < 0)
            {
                temperature = 0;
                printf("hit bottom\n");
            }

            old_position = _enc_pos;
            // send temperature to printer
        }

        if (_check_btn())
        {
            break;
        }
    }

    printf("quit set bed temp\n");
}
