//
//    FILE: DHT20_async.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: Demo for DHT20 I2C humidity & temperature sensor
//     URL: https://github.com/RobTillaart/DHT20
//
//  Always check datasheet - front view
//
//                    +--------------+
//  3V3       VDD ----| 1            |
//  D4        SDA ----| 2    DHT20   |
//  GND       GND ----| 3            |
//  D5        SCL ----| 4            |
//                    +--------------+


#include "DHT20.h"

DHT20 DHT;

uint32_t counter = 0;


void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.print("DHT20 LIBRARY VERSION: ");
  Serial.println(DHT20_LIB_VERSION);
  Serial.println();

  Wire.begin(GPIO_ID_PIN(D4), GPIO_ID_PIN(D5));
  Wire.setClock(400000);

  DHT.begin();  //  ESP32 default 21, 22

  delay(2000);

  //  start with initial request
  Serial.println(DHT.requestData());
  delay(2000);
}


void loop()
{
  uint32_t now = millis();

  if (now - DHT.lastRead() > 1000)
  {
    DHT.readData();
    DHT.convert();

    Serial.print(DHT.getHumidity(), 1);
    Serial.print(" %RH \t");
    Serial.print(DHT.getTemperature(), 1);
    Serial.print(" °C\t");
    Serial.print(counter);
    Serial.print("\n");
    //  new request
    counter = 0;
    DHT.requestData();
  }

  //  other code here
  counter++;  //  dummy counter to show async behaviour
}


//  -- END OF FILE --