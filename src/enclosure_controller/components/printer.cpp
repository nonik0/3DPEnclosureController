#include "LGFX_DinMeter.hpp"
#include "../enclosure_controller.h"

// arkanoid orange: 0xF5C396
// arakanoid dark orange: 0x754316

#define MAX_HOT_END_TEMP 295
#define MIN_HOT_END_TEMP 0
#define DEFAULT_HOT_END_TEMP 220
#define MAX_BED_TEMP 120
#define MIN_BED_TEMP 0
#define DEFAULT_BED_TEMP 60

void EnclosureController::_printer_set_extruder_temp()
{
    _printer_set_temperature(MIN_HOT_END_TEMP, MAX_HOT_END_TEMP, DEFAULT_HOT_END_TEMP, "Extruder", "M104");
}

void EnclosureController::_printer_set_bed_temp()
{
    _printer_set_temperature(MIN_BED_TEMP, MAX_BED_TEMP, DEFAULT_BED_TEMP, "Bed", "M140");
}

void EnclosureController::_printer_set_temperature(int minTemp, int maxTemp, int defaultTemp, const char* name, const char* gcode)
{
    _ssh_init();

    _canvas->setFont(&fonts::Font0);

    int temperature = defaultTemp; // TODO: get current temp from printer
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
        snprintf(string_buffer, 20, "Set %s Temp", name);
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 5);

        _canvas->setTextSize(5);
        _canvas->setTextColor((uint32_t)0x754316);
        snprintf(string_buffer, 20, "%dC", temperature);
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 55);

        _canvas_update();

        if (_check_encoder())
        {
            temperature = _enc_pos > old_position ? temperature + 5 : temperature - 5;
            if (temperature > maxTemp) temperature = maxTemp;
            if (temperature < minTemp) temperature = minTemp;

            old_position = _enc_pos;
        }

        if (_check_btn() == SHORT_PRESS)
        {
            char cmd[40];
            sprintf(cmd, "echo \"%s S%d\" > /dev/ttyAMA0", gcode, temperature);
            _ssh_cmd(cmd);
        }
        else if (_check_btn() == LONG_PRESS)
        {
            break;
        }
    }
}

