#include <Arduino_FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <FreeRTOSVariant.h>
#include <task.h>
#include <portmacro.h>
#include <semphr.h>
#include <DS1302.h>

#define LAMP_PIN        3
#define SOIL_SENSOR_PIN 8
#define WATERING_PIN    2
#define LOW_WATER_PIN   4
#define RTC_CEN_PIN     5 
#define RTC_IO_PIN      6  
#define RTC_SCLK_PIN    7 

#define WATERING_TIME_MS         4000
#define WAITING_TIME_SOIL_SENSOR 10000
#define LOW_WATER_LED_TIME_MS    3000

void WateringTask( void *pvParameters );
void LampTask( void *pvParameters );

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

  digitalWrite(LOW_WATER_PIN, LOW);
  digitalWrite(WATERING_PIN, HIGH);

  for (;;) {
    if (digitalRead(SOIL_SENSOR_PIN) == HIGH) { // если сенсор дал сигнал
      digitalWrite(WATERING_PIN, LOW); // открываем клапан
      Serial.println("Water ON");
      vTaskDelay( WATERING_TIME_MS / portTICK_PERIOD_MS  ); // поливать WATERING_TIME_MS секунд
      digitalWrite(WATERING_PIN, HIGH);
      Serial.println("Water OFF");
      vTaskDelay( WAITING_TIME_SOIL_SENSOR / portTICK_PERIOD_MS  );
      if (digitalRead(SOIL_SENSOR_PIN) == HIGH) { // если сенсор все так же дает сигнал, значит закончилась вода
        Serial.println("Low water");
        digitalWrite(WATERING_PIN, LOW); // открываем клапан
        while (digitalRead(SOIL_SENSOR_PIN) == HIGH) { // пока в емкости не появится вода, мигаем светодиодом
          digitalWrite(LOW_WATER_PIN, HIGH);
          vTaskDelay( LOW_WATER_LED_TIME_MS / portTICK_PERIOD_MS  );
          digitalWrite(LOW_WATER_PIN, LOW);
          vTaskDelay( LOW_WATER_LED_TIME_MS / portTICK_PERIOD_MS  );
          Serial.println("Low water led");
        }
        digitalWrite(WATERING_PIN, HIGH); // закрываем клапан
      }
    }
    
    vTaskDelay( 500 / portTICK_PERIOD_MS  );
  }
}

void LampTask(void *pvParameters)
{
  (void) pvParameters;

  bool lampState = false;

  for (;;) {
    Time t = rtc.time();

    if (t.hr > 8 && t.hr < 23 && !lampState) {
      Serial.println("LAMP ON");
      lampState = true;
      for (int i = 0; i < 256; ++i) { // плавное включение 20 минут
        analogWrite(LAMP_PIN, i);
        Serial.println(i);
        vTaskDelay( 4687 / portTICK_PERIOD_MS  );
      }
    }
    if (t.hr < 8 && t.hr > 23 && lampState) {
      Serial.println("LAMP OFF");
      lampState = false;
      for (int i = 255; i >= 0; --i) { // плавное выключение 20 минут
        analogWrite(LAMP_PIN, i);
        vTaskDelay( 4687 / portTICK_PERIOD_MS  );
      }
    }
    vTaskDelay( 1000 / portTICK_PERIOD_MS  );
  }
}

