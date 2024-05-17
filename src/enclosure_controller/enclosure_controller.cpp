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

    _enc_init();

    _disp_init();

    _wifi_init();

    _rtc_ntp_sync();
}