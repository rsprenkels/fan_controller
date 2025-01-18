/*
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT20.h"

const int g_fan_pwm_pin = D3;
const int g_fan_onoff_pin = D0;

unsigned long previousMillis = 0;
unsigned long loop_count = 0;
const long interval = 1000;
const int delay_millis = 20;
const int dMin = 0;
const int dMax = 255;

int g_previous_duty_cycle = -1;
unsigned long g_last_setdc = 0;

//                    +--------------+
//  3V3       VDD ----| 1            |
//  D4        SDA ----| 2    DHT20   |
//  GND       GND ----| 3            |
//  D5        SCL ----| 4            |
//                    +--------------+
DHT20 DHT;
unsigned long g_last_logged_at = 0;
float g_temperature = -100.0;
float g_humidity = -100.0;

#define SERVER_IP "192.168.2.25"
#ifndef STASSID
#define STASSID "RSJB"
#define STAPSK "Dbesbodk66"
#endif

void setup_wifi() {
  Serial.printf(PSTR("connecting to WIFI: %s\n"), STASSID);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_dht20() {
  // for the DHT20 sensor on I2C
  Wire.begin(GPIO_ID_PIN(D4), GPIO_ID_PIN(D5));
  Wire.setClock(400000);
  DHT.begin();  //  ESP32 default 21, 22
  DHT.requestData();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.print("\n\n\n");
  delay(1000);
  pinMode(g_fan_pwm_pin, OUTPUT);
  analogWriteFreq(25000);
  pinMode(g_fan_onoff_pin, OUTPUT);
  setup_dht20();
  setup_wifi();
  Serial.println(F("\nsetup complete"));
}

void set_dutycycle(int duty_cycle) {
  if (duty_cycle >= 240) {
    pinMode(g_fan_pwm_pin, INPUT);
    digitalWrite(g_fan_onoff_pin, LOW);
    Serial.printf(PSTR("duty cycle: %d (fan switched off)\n"), duty_cycle);
  } else {
    digitalWrite(g_fan_onoff_pin, HIGH);
    pinMode(g_fan_pwm_pin, OUTPUT);
    analogWriteFreq(25000);
    analogWrite(g_fan_pwm_pin, 255 - duty_cycle);
    Serial.printf(PSTR("duty cycle: %d (fan switched ON)\n"), duty_cycle);
  }
}

void set_dc_based_on_voltage() {
  uint32_t now = millis();

  if (now - g_last_setdc <= 1000) {
    return;
  } else {
    g_last_setdc = now;
  }  
  int sensor_value = analogRead(A0);
  int duty_cycle = min(255,max(0, int((sensor_value - 34) / 3.8)));
  if (g_temperature >= 22.0) {
    if (duty_cycle != g_previous_duty_cycle) {
      g_previous_duty_cycle = duty_cycle;
      set_dutycycle(duty_cycle);
    }
  } else {
      duty_cycle = 255;  
      g_previous_duty_cycle = duty_cycle;
      set_dutycycle(duty_cycle);
  }
}

void read_dht20() {
  uint32_t now = millis();

  if (now - DHT.lastRead() > 2000)
  {
    DHT.readData();
    DHT.convert();

    g_temperature = DHT.getTemperature(); 
    g_humidity = DHT.getHumidity(); 
    Serial.print(g_humidity, 1);
    Serial.print(" %RH \t");
    Serial.print(g_temperature, 1);
    Serial.print(" °C\t");
    Serial.println("");

    DHT.requestData();
  }
}

void to_log_json(char *buf, float temp, float rel_hum) {
  char s_temp[10];
  char s_rel_hum[10];
  dtostrf(temp, 2, 2, s_temp);
  dtostrf(rel_hum, 2, 2, s_rel_hum);
 
  JsonDocument t;
  t["k"] = "t";
  t["v"] = s_temp;
  JsonDocument rh;
  rh["k"] = "rh";
  rh["v"] = s_rel_hum;
  JsonDocument payload;
  payload.add(t);
  payload.add(rh);
  serializeJson(payload, buf, 100); 
}

void log_temp_humidity(float temp, float humidity) {  
  uint32_t now = millis();

  if (now - g_last_logged_at <= 5000) {
    return;
  } else {
    g_last_logged_at = now;
  }
 
  Serial.printf("WiFi status: %d\n", WiFi.status());

  if ((WiFi.status() == WL_CONNECTED)) {

    WiFiClient client;
    HTTPClient http;

    // configure traged server and url
    http.begin(client, "http://" SERVER_IP ":8000/log");  // HTTP
    http.addHeader("Content-Type", "application/json");

    // start connection and send HTTP header and body
    char payload[100];
    to_log_json(payload, temp, humidity);
    int httpCode = http.POST(payload);
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] code: %d\n", httpCode);
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
      Serial.printf("not connected to Wifi, so not logging\n");
  }
}


void loop() {
  loop_count += 1;
  set_dc_based_on_voltage();
  read_dht20();
  log_temp_humidity(g_temperature, g_humidity);
}

