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

    _ssh_init();
}