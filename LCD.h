#ifndef LCD_H
#define LCD_H


#define pcf8574_address 0x27        // default I2C address of Pcf8574
//#define pcf8574_address 0x3F        // default I2C address of Pcf8574A
#define BASE 64         // BASE is not less than 64
//////// Define the output pins of the PCF8574, which are directly connected to the LCD1602 pin.
#define RS      BASE+0
#define RW      BASE+1
#define EN      BASE+2
#define LED     BASE+3
#define D4      BASE+4
#define D5      BASE+5
#define D6      BASE+6
#define D7      BASE+7

#define delay_speed 1

#define LCD_STATUS_IDLE      0
#define LCD_STATUS_WATERING  1
#define LCD_STATUS_MOTION    2


// setup pins and I2C stuff
void setupLCD();

void cleanupLCD();

void lcdUpdateStatus(int status);

// temperature in Fahrenheit, humidity in %, water_savings in gallons
void lcdUpdateInfo(double local_temp, double local_humidity,
					double cimis_temp, double cimis_humidity,
					double local_et, double cimis_et,
					double water_savings);

// thread function to constantly display info from lcdUpdateInfo
void *lcdDisplayInfo(void *args);

#endif // LCD_H
