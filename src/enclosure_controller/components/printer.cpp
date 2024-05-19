#include <ArduinoJson.h>
#include <MD5Builder.h>
#include "LGFX_DinMeter.hpp"
#include "../enclosure_controller.h"
#include "../secrets.h"

// arkanoid orange: 0xF5C396
// arakanoid dark orange: 0x754316

#define MAX_HOT_END_TEMP 295
#define MIN_HOT_END_TEMP 0
#define MAX_BED_TEMP 120
#define MIN_BED_TEMP 0

int roundToMultiple(int number, int multiple)
{
    return ((number + multiple / 2) / multiple) * multiple;
}

String exractParam(String &authReq, const String &param, const char delimit)
{
    int _begin = authReq.indexOf(param);
    return (_begin == -1)
               ? ""
               : authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}

String getCNonce(const int len)
{
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    String s = "";
    for (int i = 0; i < len; ++i)
        s += alphanum[rand() % (sizeof(alphanum) - 1)];
    return s;
}

String generateMD5(String input)
{
    MD5Builder md5;
    md5.begin();
    md5.add(input);
    md5.calculate();
    return md5.toString();
}

String getDigestAuth(String &authReq, const String &username, const String &password, const String &method, const String &uri, unsigned int counter)
{
    String realm = exractParam(authReq, "realm=\"", '"');
    String nonce = exractParam(authReq, "nonce=\"", '"');
    String opaque = exractParam(authReq, "opaque=\"", '"');
    String cNonce = getCNonce(8);

    char nc[9];
    snprintf(nc, sizeof(nc), "%08x", counter);

    String h1 = generateMD5(username + ":" + realm + ":" + password);
    h1 = generateMD5(h1 + ":" + nonce + ":" + cNonce); // MD5-sess support
    String h2 = generateMD5(method + ":" + uri);
    String response = generateMD5(h1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":auth:" + h2);
    String authorization = "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce + "\", uri=\"" + uri +
                           "\", cnonce=\"" + cNonce + "\", nc=" + String(nc) + ", qop=auth, response=\"" + response + "\", opaque=\"" + opaque + "\", algorithm=MD5-sess";
    return authorization;
}

String getPrinterStatusJson(String &authReqHeader, unsigned int nonce)
{
    WiFiClient wifiClient;
    HTTPClient httpClient;
    String requestUri = "http://" + String(PRINTER_SERVER) + "/" + String(PRINTER_STATUS_URI);

    if (authReqHeader == nullptr || authReqHeader == "")
    {
        httpClient.begin(wifiClient, requestUri);

        const char *keys[] = {"WWW-Authenticate"};
        httpClient.collectHeaders(keys, 1);

        int httpResponseCode = httpClient.GET();
        if (httpResponseCode <= 0)
        {
            log_w("HTTP GET with no auth failed, error: %s\n", httpClient.errorToString(httpResponseCode).c_str());
            return "";
        }

        authReqHeader = httpClient.header("WWW-Authenticate");
        log_i("Received WWW-Authenticate header: %s", authReqHeader.c_str());

        httpClient.end();
    }

    String authHeader = getDigestAuth(authReqHeader, String(PRINTER_USER), String(PRINTER_PASS), "GET", "/" + String(PRINTER_STATUS_URI), nonce);

    httpClient.begin(wifiClient, "http://" + String(PRINTER_SERVER) + "/" + String(PRINTER_STATUS_URI));
    httpClient.addHeader("Authorization", authHeader);

    log_i("Sending GET with Authenticate header: %s", authHeader.c_str());
    int httpResponseCode = httpClient.GET();
    if (httpResponseCode <= 0)
    {
        log_w("HTTP GET with auth header failed, error: %s\n", httpClient.errorToString(httpResponseCode).c_str());
        return "";
    }

    String statusJson = httpClient.getString();
    httpClient.end();

    log_i("Received printer status: %s\n", statusJson.c_str());

    return statusJson;
}

void EnclosureController::_printer_get_status()
{
    log_i("Getting printer status");

    String statusJson = "";
    if (_printer_api_authreq != "")
    {
        _printer_api_nonce++;
        log_i("Using existing authreq and nonce: %d", _printer_api_nonce);
        statusJson = getPrinterStatusJson(_printer_api_authreq, _printer_api_nonce);
    }

    // if no session or session expired, get a new one
    if (statusJson == "")
    {
        _printer_api_authreq = "";
        _printer_api_nonce = 0;
        log_i("Getting new authreq and nonce");
        statusJson = getPrinterStatusJson(_printer_api_authreq, _printer_api_nonce);
    }

    if (statusJson == "")
    {
        log_e("Failed to get printer status");
        return;
    }

    DynamicJsonDocument jsonDoc(1024);
    DeserializationError jsonError = deserializeJson(jsonDoc, statusJson);
    if (jsonError)
    {
        log_e("deserializeJson() failed: %s", jsonError.c_str());
        return;
    }

    _printer_extruder_temp = jsonDoc["printer"]["temp_nozzle"];
    _printer_bed_temp = jsonDoc["printer"]["temp_bed"];

    log_i("Printer extruder temp: %f", _printer_extruder_temp);
    log_i("Printer bed temp: %f", _printer_bed_temp);
}

void EnclosureController::_printer_set_extruder_temp()
{
    _printer_get_status();
    _printer_set_temperature(_printer_extruder_temp, MIN_HOT_END_TEMP, MAX_HOT_END_TEMP, "Extruder", "M104");
}

void EnclosureController::_printer_set_bed_temp()
{
    _printer_get_status();
    _printer_set_temperature(_printer_bed_temp, MIN_BED_TEMP, MAX_BED_TEMP, "Bed", "M140");
}

void EnclosureController::_printer_set_temperature(int curTemp, int minTemp, int maxTemp, const char *name, const char *gcode)
{
    _ssh_init();

    _canvas->setFont(&fonts::Font0);

    int temperature = curTemp;
    long old_position = 0;
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
            if (temperature % 5 != 0)
                temperature = _enc_pos > old_position ? ((temperature + 5) / 5) * 5 : (temperature / 5) * 5;
            else
                temperature = _enc_pos > old_position ? temperature + 5 : temperature - 5;

            if (temperature > maxTemp)
                temperature = maxTemp;
            if (temperature < minTemp)
                temperature = minTemp;

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
