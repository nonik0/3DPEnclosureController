#include <ArduinoJson.h>
#include <MD5Builder.h>
#include "LGFX_DinMeter.hpp"
#include "../enclosure_controller.h"
#include "../secrets.h"

// arkanoid orange: 0xF5C396
// arakanoid dark orange: 0x754316

#define PRINTER_STATUS_URI "api/v1/status"
#define PRINTER_JOB_URI "api/v1/job"
#define MAX_HOT_END_TEMP 295
#define DEFAULT_HOT_END_TEMP 225
#define MIN_HOT_END_TEMP 0
#define MAX_BED_TEMP 120
#define DEFAULT_BED_TEMP 60
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

String EnclosureController::_printer_send_api_request(String httpMethod, String requestUri)
{
    WiFiClient wifiClient;
    HTTPClient httpClient;
    String requestUrl = "http://" + String(PRINTER_SERVER) + "/" + requestUri;

    bool existingSession = _printer_api_authreq != nullptr && _printer_api_authreq != "";
    int attemptsCount = existingSession ? 2 : 1; // attempt twice in case existing session is expired
    for (int attempts = 0; attempts < attemptsCount; attempts++)
    {
        if (!existingSession)
        {
            httpClient.begin(wifiClient, requestUrl);

            const char *keys[] = {"WWW-Authenticate"};
            httpClient.collectHeaders(keys, 1);

            int httpResponseCode = httpClient.GET();
            if (httpResponseCode <= 0)
            {
                log_w("HTTP GET with no auth failed, error: %s\n", httpClient.errorToString(httpResponseCode).c_str());
                return "";
            }

            _printer_api_authreq = httpClient.header("WWW-Authenticate");
            log_d("Received WWW-Authenticate header: %s", _printer_api_authreq.c_str());

            httpClient.end();
        }

        // generate auth header for request
        String authHeader = getDigestAuth(_printer_api_authreq, String(PRINTER_USER), String(PRINTER_PASS), httpMethod, "/" + requestUri, _printer_api_nonce);

        httpClient.begin(wifiClient, requestUrl);
        httpClient.addHeader("Authorization", authHeader);

        log_i("Sending %s to '%s'", httpMethod.c_str(), requestUrl.c_str());
        log_d("With Authenticate header:\n%s", authHeader.c_str());

        int httpResponseCode;
        if (httpMethod == "GET")
            httpResponseCode = httpClient.GET();
        else if (httpMethod == "PUT")
            httpResponseCode = httpClient.PUT("");
        else if (httpMethod == "DELETE")
            httpResponseCode = httpClient.DELETE();
        else
        {
            log_e("Unsupported HTTP method: %s", httpMethod.c_str());
            return "";
        }

        if (httpResponseCode > 0 && httpResponseCode != 401)
            _printer_api_nonce++;

        if (httpResponseCode >= 200 && httpResponseCode < 300)
            break;

        log_w("HTTP %s with auth header failed %d, will clear session. Error: %s\n", httpMethod, httpResponseCode, httpClient.errorToString(httpResponseCode).c_str());
        _printer_api_authreq = "";
        _printer_api_nonce = 0;
    }

    String response = httpClient.getString();
    httpClient.end();

    log_i("Received printer API response:\n%s\n", response.c_str());
    return response;
}

void EnclosureController::_printer_update_status()
{
    String statusJson = _printer_send_api_request("GET", PRINTER_STATUS_URI);
    JsonDocument jsonDoc;
    DeserializationError jsonError = deserializeJson(jsonDoc, statusJson);
    if (jsonError)
    {
        log_e("deserializeJson() failed: %s", jsonError.c_str());
        return;
    }

    const char *state = jsonDoc["printer"]["state"];
    _printer_status.state = String(state);
    _printer_status.temp_nozzle = jsonDoc["printer"]["temp_nozzle"];
    _printer_status.temp_bed = jsonDoc["printer"]["temp_bed"];
    _printer_status.speed = jsonDoc["printer"]["speed"];
    _printer_status.target_nozzle = jsonDoc["printer"]["target_nozzle"];
    _printer_status.target_bed = jsonDoc["printer"]["target_bed"];

    if (jsonDoc.containsKey("job"))
    {
        _printer_job.id = jsonDoc["job"]["id"];
        _printer_job.progress = jsonDoc["job"]["progress"];
        _printer_job.time_remaining = jsonDoc["job"]["time_remaining"];
    }
    else
    {
        _printer_job.id = 0;
        _printer_job.progress = 0;
        _printer_job.time_remaining = 0;
    }

    log_i("Printer status: State: %s, Nozzle Temp: %f, Bed Temp: %f, Speed: %f, Target Nozzle Temp: %f, Target Bed Temp: %f",
          _printer_status.state.c_str(), _printer_status.temp_nozzle, _printer_status.temp_bed, _printer_status.speed, _printer_status.target_nozzle, _printer_status.target_bed);
    log_i("Printer job: ID: %d, Progress: %d, Time Remaining: %d", _printer_job.id, _printer_job.progress, _printer_job.time_remaining);
}

void EnclosureController::_printer_set_extruder_temp()
{
    _printer_update_status();

    float displayTemp = _printer_status.target_nozzle > 0 ? _printer_status.target_nozzle : DEFAULT_HOT_END_TEMP; 
    _printer_set_temperature(displayTemp, MIN_HOT_END_TEMP, MAX_HOT_END_TEMP, "Extruder", "M104");
}

void EnclosureController::_printer_set_bed_temp()
{
    _printer_update_status();

    float displayTemp = _printer_status.target_bed > 0 ? _printer_status.target_bed : DEFAULT_BED_TEMP;
    _printer_set_temperature(displayTemp, MIN_BED_TEMP, MAX_BED_TEMP, "Bed", "M140");
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

void EnclosureController::_printer_manage_job()
{
    static String ActiveJobOptions[] = {"Pause", "Stop"};
    static String PausedJobOptions[] = {"Resume", "Stop"};

    _printer_update_status();

    _canvas->setFont(&fonts::Font0);

    unsigned long lastUpdate = millis();
    _enc_pos = 0;
    _enc.setPosition(_enc_pos);

    while (1)
    {
        _canvas->fillScreen((uint32_t)0xF5C396);

        _canvas->fillRect(0, 0, 240, 25, (uint32_t)0x754316);
        _canvas->setTextSize(2);
        _canvas->setTextColor((uint32_t)0xF5C396);
        _canvas->drawCenterString("Manage Print Job", _canvas->width() / 2, 5);

        _canvas->setTextSize(5);
        _canvas->setTextColor((uint32_t)0x754316);

        if (_printer_job.id != 0)
        {
            char string_buffer[20];
            if (_printer_status.state == "PRINTING")
            {
                if (_enc_pos % 2 == 0)
                {
                    snprintf(string_buffer, 20, "Pause");
                }
                else
                {
                    snprintf(string_buffer, 20, "Stop");
                }
            }
            else if (_printer_status.state == "PAUSED")
            {
                if (_enc_pos % 2 == 0)
                {
                    snprintf(string_buffer, 20, "Resume");
                }
                else
                {
                    snprintf(string_buffer, 20, "Stop");
                }
            }
            else
            {
                snprintf(string_buffer, 20, "%s", _printer_status.state);
            }
            _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 55);

            // print progress % and time
            snprintf(string_buffer, 20, "%d%% done, %dm left", _printer_job.progress, _printer_job.time_remaining / 60);
            _canvas->setTextSize(2);
            _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 110);
        }
        else
        {
            _canvas->drawCenterString(_printer_status.state, _canvas->width() / 2, 55);
        }

        // periodiocally check for printer status updates
        if (millis() - lastUpdate > 5000)
        {
            _printer_update_status();
            lastUpdate = millis();
        }

        _canvas_update();
        _check_encoder();

        if (_check_btn() == SHORT_PRESS)
        {
            if (_printer_status.state == "PRINTING")
            {
                if (_enc_pos % 2 == 0)
                    _printer_send_api_request("PUT", String(PRINTER_JOB_URI) + "/" + String(_printer_job.id) + "/pause");
                else
                    _printer_send_api_request("DELETE", String(PRINTER_JOB_URI) + "/" + String(_printer_job.id));
            }
            else if (_printer_status.state == "PAUSED")
            {
                if (_enc_pos % 2 == 0)
                    _printer_send_api_request("PUT", String(PRINTER_JOB_URI) + "/" + String(_printer_job.id) + "/resume");
                else
                    _printer_send_api_request("DELETE", String(PRINTER_JOB_URI) + "/" + String(_printer_job.id));
            }

            _printer_update_status();
        }
        else if (_check_btn() == LONG_PRESS)
        {
            break;
        }
    }
}