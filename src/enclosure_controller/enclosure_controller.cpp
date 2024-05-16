/**
 * @file factory_test.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-06-06
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "enclosure_controller.h"

void EnclosureController::init()
{
    _power_on();

    _rtc_init();

    _btn_init();

    _disp_init();

    // Encoder init
    _enc.attachHalfQuad(40, 41);

    _enc.setCount(0);

    _wifi_init();
}