/*
  Ron Sprenkels
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT20.h"

int ledState = LOW;
const uint8_t pwm_fan_pin = D3;
const uint8_t tacho_fan_pin = D2; 

unsigned long previousMillis = 0;
unsigned long loop_count = 0;
const long interval = 1000;
const int delay_millis = 20;
const int dMin = 0;
const int dMax = 255;

unsigned long g_last_logged_at = 0;
float g_temperature = -100.0;
float g_humidity = -100.0;

//                    +--------------+
//  3V3       VDD ----| 1            |
//  D4        SDA ----| 2    DHT20   |
//  GND       GND ----| 3            |
//  D5        SCL ----| 4            |
//                    +--------------+
DHT20 DHT;

int previous_duty_cycle = -1;

int volatile tacho_pulse_count = 0;
unsigned long tacho_measure_start_time;
int fan_rpm = 0;

#define SERVER_IP "192.168.2.25"
#ifndef STASSID
#define STASSID "BE"
#define STAPSK "Dbesbodk66"
#endif

void setup_wifi() {
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(pwm_fan_pin, OUTPUT);
  analogWriteFreq(25000);
  pinMode(tacho_fan_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(tacho_fan_pin), tachoCounter, RISING);

  // for the DHT20 sensor on I2C
  Wire.begin(GPIO_ID_PIN(D4), GPIO_ID_PIN(D5));
  Wire.setClock(400000);
  DHT.begin();  //  ESP32 default 21, 22
  Serial.println(DHT.requestData());

  Serial.begin(115200);
  Serial.println(F("\nFan controller and data logger"));

  setup_wifi();
}


void change_dutycycle() {
  for(int dutyCycle = dMin; dutyCycle <= dMax; dutyCycle++){   
    // changing the LED brightness with PWM
    Serial.printf(PSTR("duty cycle %d\n"), dutyCycle);
    analogWrite(pwm_fan_pin, dutyCycle);
    delay(delay_millis);
  }
  delay(500);
  // decrease the LED brightness
  for(int dutyCycle = dMax; dutyCycle >= dMin; dutyCycle--){
    // changing the LED brightness with PWM
    Serial.printf(PSTR("duty cycle %d\n"), dutyCycle);
    analogWrite(pwm_fan_pin, dutyCycle);
    delay(delay_millis);
  }
  delay(500);
}

void set_dutycycle(int duty_cycle) {
  analogWrite(pwm_fan_pin, 255 - duty_cycle);
  Serial.printf(PSTR("duty cycle: %d\n"), duty_cycle);
}

int set_dc_based_on_voltage() {
  int sensor_value = get_analog_reading();
  int duty_cycle = min(255,max(0, int((sensor_value - 34) / 3.8)));
  if (duty_cycle != previous_duty_cycle) {
    previous_duty_cycle = duty_cycle;
    set_dutycycle(duty_cycle);
  }
  return duty_cycle;
}

int set_dc_based_on_temperature() {
  int duty_cycle;
  if (g_temperature > 23.0) {
    duty_cycle = set_dc_based_on_voltage();
  } else {
    duty_cycle = 255;
  }
  if (duty_cycle != previous_duty_cycle) {
    previous_duty_cycle = duty_cycle;
    set_dutycycle(duty_cycle);
  }
  return duty_cycle;
}

int get_analog_reading() {
  int sensorValue = analogRead(A0);
  return sensorValue;
}

ICACHE_RAM_ATTR void tachoCounter() {
  tacho_pulse_count++;
}

void measure_rpm() {
    for(int loop_cnt = 0; loop_cnt <= 1; loop_cnt++) {
      tacho_measure_start_time = millis();
      tacho_pulse_count = 0;
      while ((millis() - tacho_measure_start_time) < 1000) {
        // void
      }
      fan_rpm = tacho_pulse_count * 60 / 2;
      Serial.printf(PSTR("Fan running at: %d rpm.\n"), fan_rpm);
  }
}

void read_dht20() {
  uint32_t now = millis();

  if (now - DHT.lastRead() > 1000)
  {
    DHT.readData();
    DHT.convert();

    Serial.print(DHT.getHumidity(), 1);
    Serial.print(" %RH \t");
    g_temperature = DHT.getTemperature(); 
    g_humidity = DHT.getHumidity(); 
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

  if (now - g_last_logged_at <= 10000) {
    return;
  } else {
    g_last_logged_at = now;
  }
 
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
  }
}


void loop() {
  loop_count += 1;
  set_dc_based_on_voltage();
  // set_dc_based_on_temperature();
  measure_rpm();
  read_dht20();
  log_temp_humidity(g_temperature, g_humidity);
}

