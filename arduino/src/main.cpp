#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char *ssid = "SSID_HERE";
const char *password = "PASSWORD_HERE";
String server = "http://192.168.86.230:8080/sensorUpdate";

void setup()
{
  while (!Serial)
    ;
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }
  Serial.println("Connected to wifi network!");
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String get_request = server + "?temperature=" + random(40);
    // Your Domain name with URL path or IP address with path
    http.begin(get_request.c_str());

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  delay(5000);
}