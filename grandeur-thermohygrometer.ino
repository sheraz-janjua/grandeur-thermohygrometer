#include <DHT.h>         // sensor library
#include <Grandeur.h>    // cloud sdk library
#include <ESP8266WiFi.h> // Wi-Fi library
// to include credentials
// in a separate file
// uncomment following line
// and be sure to define SECRETS macro in it
// #include "secrets.h"
// otherwise add them in below ifdef block
#ifndef SECRETS
// Cloud credentials
const char *apiKey = "---grandeur-cloud-project-api-key------";
const char *deviceID = "-device-id-created-by-grandeur-cloud-";
const char *token = "----token-generated-by-grandeur-cloud-on-device-registration-";
// Wi-Fi credentials
const char *sta_ssid = "********";
const char *sta_pass = "********";
#endif

#define WIFI_TIMEOUT 5000
// sensor pin and model definition
#define SENSOR_PIN 2
#define SENSOR_MODEL DHT11
// period in milliseconds after which sensor is polled
#define POLL_PERIOD 500
// global variables
long t = 0; // to cache last poll time instant in terms of millis()
bool param = false;        // switch between temperature and humidity
float old_val[2] = {0, 0}; // cached readings for detecting change
wl_status_t old_wifi_status = WL_IDLE_STATUS; // cached Wi-Fi connection status
bool cloud_connected = false; // Cloud connection status

// sensor instance
static DHT sensor(SENSOR_PIN, SENSOR_MODEL);
// Grandeur project instance
Grandeur::Project project;

// function declarations

void init_wifi(); // Wi-Fi initialization
void onConnection(bool _status); // cloud connection callback

// system initialization 
void setup()
{
  // Serial UART port initialization
  Serial.begin(115200);
  // sensor initialization
  sensor.begin();
  // Wi-Fi initialization
  while (WiFi.status() != WL_CONNECTED)
    init_wifi();
  Serial.printf("apiKey= %s\n token= %s\n deviceID = %s\n", apiKey, token, deviceID);
  // Grandeur cloud SDK initialization
  project = grandeur.init(apiKey, token);
  // set cloud connection status callback
  project.onConnection(onConnection);
}
                
// infinite/forever loop
void loop()
{
  wl_status_t _wifi_status = WiFi.status();// check Wi-Fi connection
  // prompt on Wi-Fi connection status change
  if (old_wifi_status != _wifi_status)
  {
    old_wifi_status = _wifi_status;
    Serial.printf("Wi-Fi %sconnected\n", (_wifi_status == WL_CONNECTED ? "" : "dis"));
  }
  // poll sensor periodically
  if (millis() - t > POLL_PERIOD)
  {
    t = millis(); // cache last poll time instant
    // get sensor value
    float val = (param) ? sensor.readHumidity() : sensor.readTemperature();
    if (!isnan(val))
    {
      // if value is valid
      // and differs by 0.1
      // from its previous value
      // post it to cloud
      if (fabs(val - old_val[param]) > 0.01)
      {
        old_val[param] = val; // cache the value for future comparison
        char buff[6];
        sprintf(buff, "%.2f", val);
        if (cloud_connected)
          project.device(deviceID).data().set((param ? "h" : "t"), buff);
        Serial.printf("%s = %s\n", (param ? "humidity" : "temperature"), buff);
      }
    }
    // toggle (humidity/temperature) parameter
    param = !param;
  }
  // run the Grandeur project loop
  // to synchronize device data with cloud
  // provided WiFi is connected
  project.loop(_wifi_status == WL_CONNECTED);
}

// function definitions

// Grandeur cloud connection status callback
void onConnection(bool _status)
{
  Serial.printf("cloud %sconnected\n", (_status ? "" : "dis"));
  cloud_connected = _status;
}

// Wi-Fi initialization
void init_wifi()
{
  bool connection_success = false;
  unsigned long _time_start = millis();
  // station mode to connect to modem
  //  WiFi.mode(WIFI_STA);
  // attempt connection
  Serial.printf("connecting to ssid:%s\n with pass:%s\n", sta_ssid, sta_pass);
  WiFi.begin(sta_ssid, sta_pass);
  // wait a while for Wi-Fi connection to establish
  while (WiFi.status() != WL_CONNECTED && (millis() - _time_start < WIFI_TIMEOUT))
  {
    delay(500);
    Serial.println(".");
  }
  connection_success = WiFi.status() == WL_CONNECTED;
  Serial.printf("WiFi %sconnected\n", (connection_success  ? "" : "not "));
  if (connection_success)
  {
    WiFi.setAutoReconnect(true); // maintain Wi-Fi connection automatically
    WiFi.persistent(true); // save Wi-Fi settings
    Serial.println(WiFi.localIP()); // show assigned ip address
  }
}
