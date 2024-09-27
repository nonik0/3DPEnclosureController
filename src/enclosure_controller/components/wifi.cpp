#include "../enclosure_controller.h"
#include "../secrets.h"

void EnclosureController::_wifi_init()
{
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint8_t wifiAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20)
    {
      Serial.print(".");
      delay(1000);
      if (wifiAttempts == 10)
      {
        WiFi.disconnect(true, true);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
      }
      wifiAttempts++;
    }

    log_w("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  log_i("IP: %s", WiFi.localIP().toString().c_str());

  Serial.println("OTA setting up...");

  // ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("3DP-Enclosure-Controller");

  ArduinoOTA
      .onStart([]()
               {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else  // U_SPIFFS
          type = "filesystem";

        Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed"); });
  ArduinoOTA.begin();

  Serial.println("OTA setup complete.");

  if (!MDNS.begin("3dp-enclosure-controller"))
  {
    Serial.println("Error setting up MDNS responder!");

    return;
  }
  Serial.println("mDNS responder started");

  _wifi_inited = true;
}

bool EnclosureController::_wifi_check()
{
  if (!_wifi_inited)
  {
    return false;
  }

  try
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      log_w("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      // wifiDisconnects++;
      log_w("Reconnected to WiFi");
    }
  }
  catch (const std::exception &e)
  {
    log_e("Wifi error: %s", e.what());
    return false;
  }

  return true;
}