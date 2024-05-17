#include "../enclosure_controller.h"

const char* NtpServer = "pool.ntp.org";
const long GmtOffsetSecs = -28800;
const int DstOffsetSecs = 3600;

void EnclosureController::_rtc_init()
{
    Wire.begin(11, 12, 100000);

    _rtc.begin();
    _rtc.clearIRQ();
    _rtc.disableIRQ();
}

void EnclosureController::_rtc_ntp_sync()
{
    if (!_wifi_check())
    {
        log_w("No WiFi connection");
        return;
    }

    struct tm timeinfo;
    configTime(GmtOffsetSecs, DstOffsetSecs, NtpServer);
    getLocalTime(&timeinfo);

    int yr = timeinfo.tm_year + 1900;
    int mt = timeinfo.tm_mon + 1;
    int dy = timeinfo.tm_mday;
    int hr = timeinfo.tm_hour;
    int mi = timeinfo.tm_min;
    int se = timeinfo.tm_sec;

    I2C_BM8563_TimeTypeDef time = {hr, mi, se};
    I2C_BM8563_DateTypeDef date = {timeinfo.tm_wday, mt, dy, yr};

    _rtc.setDate(&date);
    _rtc.setTime(&time);

    _rtc_check();
}

void EnclosureController::_rtc_check()
{
    I2C_BM8563_DateTypeDef date;
    I2C_BM8563_TimeTypeDef time;

    _rtc.getDate(&date);
    _rtc.getTime(&time);

    log_i("RTC: %d-%d-%d %d:%d:%d", date.year, date.month, date.date, time.hours, time.minutes, time.seconds);
}