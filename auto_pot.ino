#include <Arduino_FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <FreeRTOSVariant.h>
#include <task.h>
#include <portmacro.h>
#include <semphr.h>
#include <DS1302.h>

#define LAMP_PIN        3  // вывод ШИМ лампы освещения
#define SOIL_SENSOR_PIN 8  // вывод датчика влажности
#define WATERING_PIN    2  // вывод клапана полива
#define LOW_WATER_PIN   4  // вывод светодиода низкого уровня воды
#define RTC_CEN_PIN     5  // выводы часов реального времени
#define RTC_IO_PIN      6  
#define RTC_SCLK_PIN    7  

#define WATERING_TIME_MS         7000    // время полива мс
#define WAITING_TIME_SOIL_SENSOR 10000   // время ожидания после полива
#define LOW_WATER_LED_TIME_MS    3000    // период мигания светодиода

#define LAMP_ON_TIME_HR     8    // время включения лампы
#define LAMP_OFF_TIME_HR    23   // время выключения лампы
#define LAMP_RISE_TIME_MIN  20   // время плавного нарастания лампы
#define LAMP_RISE_TIME_MS   (LAMP_RISE_TIME_MIN * 60000)

#define LAMP_OFF            255      
#define LAMP_MAX_BRIGHTNESS 0
#define LAMP_MIN_BRIGHTNESS 47
#define LAMP_RANGE          LAMP_MIN_BRIGHTNESS - LAMP_MAX_BRIGHTNESS

class Pot {

  bool m_lampState;

public:

  Pot() : m_lampState(false) 
  {
    digitalWrite(WATERING_PIN, HIGH);
    digitalWrite(LOW_WATER_PIN, LOW);
    analogWrite(LAMP_PIN, LAMP_OFF);
  }

  bool isSoilDry() 
  {
    if (digitalRead(SOIL_SENSOR_PIN) == HIGH) {
      Serial.println("Soil is dry");
      return true;
    } else return false;
  }

  void valveOn() 
  {
    digitalWrite(WATERING_PIN, LOW);
    Serial.println("Valve ON");
  }

  void valveOff() 
  {
    digitalWrite(WATERING_PIN, HIGH);
    Serial.println("Valve OFF");
  }

  void blinkLowWaterLed() 
  {
    Serial.println("Low water");
    digitalWrite(LOW_WATER_PIN, HIGH);
    vTaskDelay( LOW_WATER_LED_TIME_MS / portTICK_PERIOD_MS  );
    digitalWrite(LOW_WATER_PIN, LOW);
    vTaskDelay( LOW_WATER_LED_TIME_MS / portTICK_PERIOD_MS  );
  }

  bool lampState() 
  {
    return m_lampState;
  }

  void lampOn() 
  {
    Serial.println("Lamp ON");
    m_lampState = true;
    for (int i = LAMP_MIN_BRIGHTNESS; i >= LAMP_MAX_BRIGHTNESS; --i) {
      analogWrite(LAMP_PIN, i);
      vTaskDelay( (LAMP_RISE_TIME_MS / LAMP_RANGE) / portTICK_PERIOD_MS  );
    } 
  }

  void lampOff() 
  {
    Serial.println("Lamp OFF");
    m_lampState = false;
    for (int i = LAMP_MAX_BRIGHTNESS; i <= LAMP_MIN_BRIGHTNESS; ++i) {
      analogWrite(LAMP_PIN, i);
      vTaskDelay( (LAMP_RISE_TIME_MS / LAMP_RANGE) / portTICK_PERIOD_MS  );
      Serial.println(i);
    }
    analogWrite(LAMP_PIN, LAMP_OFF);
  }
};

void WateringTask( void *pvParameters );
void LampTask( void *pvParameters );

Pot pot;

DS1302 rtc(RTC_CEN_PIN, RTC_IO_PIN, RTC_SCLK_PIN);

void setup() {

  Serial.begin(9600);

  pinMode(LAMP_PIN, OUTPUT);
  pinMode(SOIL_SENSOR_PIN, INPUT);
  pinMode(WATERING_PIN, OUTPUT);
  pinMode(LOW_WATER_PIN, OUTPUT);

  xTaskCreate(
    WateringTask
    ,  (const portCHAR *) "Управление поливом"
    ,  128
    ,  NULL
    ,  2
    ,  NULL );


  xTaskCreate(
    LampTask
    ,  (const portCHAR *) "Управление освещением"
    ,  128
    ,  NULL
    ,  1
    ,  NULL );

}

void loop()
{

}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void WateringTask(void *pvParameters)
{
  (void) pvParameters;

  for (;;) {
    if (pot.isSoilDry()) {
      pot.valveOn();
      vTaskDelay( WATERING_TIME_MS / portTICK_PERIOD_MS  ); // поливать WATERING_TIME_MS секунд
      pot.valveOff();
      vTaskDelay( WAITING_TIME_SOIL_SENSOR / portTICK_PERIOD_MS  );
      if (pot.isSoilDry()) { // если сенсор все так же дает сигнал, значит закончилась вода
        pot.valveOn();
        while (pot.isSoilDry())
          pot.blinkLowWaterLed();
        pot.valveOff();
      }
    }
    vTaskDelay( 500 / portTICK_PERIOD_MS  );
  }
}

void LampTask(void *pvParameters)
{
  (void) pvParameters;

  for (;;) {
    Time t = rtc.time();

    if (t.hr >= LAMP_ON_TIME_HR && t.hr < LAMP_OFF_TIME_HR && !pot.lampState()) {
      pot.lampOn();
    }
    if (t.hr < LAMP_ON_TIME_HR && t.hr >= LAMP_OFF_TIME_HR && pot.lampState()) {
      pot.lampOff();
    }
    vTaskDelay( 1000 / portTICK_PERIOD_MS  );
  }
}

