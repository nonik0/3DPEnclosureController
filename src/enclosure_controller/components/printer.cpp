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

String exractParam(String &authReq, const String &param, const char delimit)
{
    int _begin = authReq.indexOf(param);
    if (_begin == -1)
    {
        return "";
    }
    return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}

String getCNonce(const int len)
{
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    String s = "";
    for (int i = 0; i < len; ++i)
    {
        s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
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
    // extracting required parameters for RFC 2069 simpler Digest
    String realm = exractParam(authReq, "realm=\"", '"');
    String nonce = exractParam(authReq, "nonce=\"", '"');
    String opaque  = exractParam(authReq, "opaque=\"", '"');
    String cNonce = getCNonce(8);

    char nc[9];
    snprintf(nc, sizeof(nc), "%08x", counter);

    // parameters for the RFC 2617 newer Digest
    String h1 = generateMD5(username + ":" + realm + ":" + password);
    String h2 = generateMD5(method + ":" + uri);
    String response = generateMD5(h1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":" + "auth" + ":" + h2);
    //String response = generateMD5(ha1 + ":" + nonce + ":" + "00000001:xyz:auth:" + ha2);

    String authorization = "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce + "\", uri=\"" + uri +
        ", cnonce=\"" + cNonce + "\" + nc=" + String(nc) + ", qop=auth," + "\", response=\"" + response + "\"";
    return authorization;
}

String getPrinterStatus()
{
    WiFiClient wifiClient;
    HTTPClient httpClient;
    String requestUri = "http://" + String(PRINTER_SERVER) + "/" + String(PRINTER_URI);
    httpClient.begin(wifiClient, requestUri);

    const char *keys[] = {"WWW-Authenticate"};
    httpClient.collectHeaders(keys, 1);

    int httpResponseCode = httpClient.GET();
    if (httpResponseCode != 401)
    {
        log_w("HTTP GET not 401 response, error: %s\n", httpClient.errorToString(httpResponseCode).c_str());
        return "";
    }

    String authReq = httpClient.header("WWW-Authenticate");
    log_i("Received WWW-Authenticate header: %s", authReq.c_str());

    String authorization = getDigestAuth(authReq, String(PRINTER_USER), String(PRINTER_PASS), "GET", "/" + String(PRINTER_URI), 1);

    httpClient.end();
    httpClient.begin(wifiClient, "http://" + String(PRINTER_SERVER) + "/" + String(PRINTER_URI));
    httpClient.addHeader("Authorization", authorization);

    log_i("Sending GET with Authenticate header: %s", authorization.c_str());
    httpResponseCode = httpClient.GET();
    if (httpResponseCode <= 0)
    {
        log_w("HTTP GET failed, error: %s\n", httpClient.errorToString(httpResponseCode).c_str());
        return "";
    }

    String payload = httpClient.getString();
    log_i("HTTP Response code: %d", httpResponseCode);
    log_i("HTTP Response: %s", payload.c_str());

    httpClient.end();

    return payload;
}

void EnclosureController::_printer_get_status()
{
    log_i("Getting printer status");
    getPrinterStatus();
}

void EnclosureController::_printer_set_extruder_temp()
{
    _printer_get_status();
    _printer_set_temperature(_extruder_temp, MIN_HOT_END_TEMP, MAX_HOT_END_TEMP, "Extruder", "M104");
}

void EnclosureController::_printer_set_bed_temp()
{
    _printer_get_status();
    _printer_set_temperature(_bed_temp, MIN_BED_TEMP, MAX_BED_TEMP, "Bed", "M140");
}

void EnclosureController::_printer_set_temperature(int curTemp, int minTemp, int maxTemp, const char *name, const char *gcode)
{
    _ssh_init();

    _canvas->setFont(&fonts::Font0);

    int temperature = curTemp; // TODO: get current temp from printer
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
