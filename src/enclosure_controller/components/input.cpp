#include "../enclosure_controller.h"

void EnclosureController::_power_on()
{
    /* Hold pwr pin */
    gpio_reset_pin((gpio_num_t)POWER_HOLD_PIN);
    pinMode(POWER_HOLD_PIN, OUTPUT);
    digitalWrite(POWER_HOLD_PIN, HIGH);
}

void EnclosureController::_power_off()
{
    _rtc.clearIRQ();
    _rtc.disableIRQ();

    _disp->fillScreen(TFT_BLACK);

    while ((!_enc_btn.read()))
    {
        delay(100);
    }

    delay(200);

    printf("power off\n");
    digitalWrite(POWER_HOLD_PIN, 0);
    delay(10000);

    while (1)
    {
        delay(1000);
    }
}

void EnclosureController::_btn_init()
{
    _enc_btn.begin();
}

EnclosureController::PressType EnclosureController::_check_btn()
{
    if (!_enc_btn.read())
    {
        _tone(2500, 50);

        uint8_t time_count = 0;
        while (!_enc_btn.read())
        {
            time_count++;
            if (time_count > 20)
            {
                return PressType::LONG_PRESS;
            }

            delay(10);
        }

        return PressType::SHORT_PRESS;
    }

    return PressType::NONE;
}

void EnclosureController::_wait_next()
{
    while (!_check_btn())
    {
        _check_encoder();
        delay(10);
    }
    printf("go next\n");
}

void EnclosureController::_enc_init()
{
    // Encoder init
    _enc.attachHalfQuad(40, 41);

    _enc.setCount(0);
}

bool EnclosureController::_check_encoder(bool playBuzz)
{
    if (_enc_pos != _enc.getPosition())
    {
        /* Bi */
        if (playBuzz)
        {
            _noTone();
            // _tone(((_enc.getDirection() == RotaryEncoder::Direction::CLOCKWISE) ? 3000 : 3500), 20);
            _tone(((_enc.getPosition() > _enc_pos) ? 3000 : 3500), 20);
            // delay(20);
        }

        _enc_pos = _enc.getPosition();

        return true;
    }

    return false;
}
