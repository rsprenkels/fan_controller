/*
  Ron Sprenkels
*/

int ledState = LOW;
const uint8_t pwm_fan_pin = D3;
const uint8_t tacho_fan_pin = D2; 

unsigned long previousMillis = 0;
unsigned long loop_count = 0;
const long interval = 1000;
const int delay_millis = 20;
const int dMin = 0;
const int dMax = 255;

float g_temperature = -100.0;

//                    +--------------+
//  3V3       VDD ----| 1            |
//  D4        SDA ----| 2    DHT20   |
//  GND       GND ----| 3            |
//  D5        SCL ----| 4            |
//                    +--------------+
#include "DHT20.h"
DHT20 DHT;

int previous_duty_cycle = -1;

int volatile tacho_pulse_count = 0;
unsigned long tacho_measure_start_time;
int fan_rpm = 0;

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
  Serial.println(F("\nPWM generation example"));
}

// void loop() {
//   unsigned long currentMillis = millis();
//   if (currentMillis - previousMillis >= interval) {
//     previousMillis = currentMillis;
//     if (ledState == LOW) {
//       ledState = HIGH;  // Note that this switches the LED *off*
//     } else {
//       ledState = LOW;  // Note that this switches the LED *on*
//     }
//     digitalWrite(LED_BUILTIN, ledState);
//     Serial.printf(PSTR("idle loop count %d\n"), idleLoopCount);
//     idleLoopCount = 0;
//   } else {
//     idleLoopCount += 1;
//   }
// }

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
    Serial.print(g_temperature, 1);
    Serial.print(" °C\t");
    Serial.println("");
    DHT.requestData();
  }
}

void loop() {
  loop_count += 1;
  set_dc_based_on_voltage();
  // set_dc_based_on_temperature();
  measure_rpm();
  read_dht20();
}

