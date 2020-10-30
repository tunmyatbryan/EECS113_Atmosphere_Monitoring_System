#include "LCD.h"

#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <pcf8574.h>
#include <lcd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

// the main loop condition is always true if compiled with DEBUG
// otherwise, it's a global variable set elsewhere
#ifdef DEBUG
int global_run_lcd = 1;
#else
extern int global_run_lcd;
#endif // DEBUG

#define DISPLAY_LEN 16

#define MIN(a,b) (((a)<(b))?(a):(b))

// the strings to be displayed on the LCD
char status_str[DISPLAY_LEN + 1];
char info_str[200];

// length of each string to be displayed
int status_len;
int info_len;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int lcdhd;// used to handle LCD

void setupLCD(){
    int i;
    pcf8574Setup(BASE,pcf8574_address);// initialize PCF8574
    for(i=0;i<8;i++){
        pinMode(BASE+i,OUTPUT);     // set PCF8574 port to output mode
    } 
    digitalWrite(LED,HIGH);     // turn on LCD backlight
    digitalWrite(RW,LOW);       // allow writing to LCD
    
    lcdhd = lcdInit(2,16,4,RS,EN,D4,D5,D6,D7,0,0,0,0);// initialize LCD and return handle used to handle LCD
    
    if(lcdhd == -1){
        printf("lcdInit failed !");
        return;
    }

	status_len = 0;
	info_len = 0;

	lcdClear(lcdhd);
}

void cleanupLCD(){
	lcdClear(lcdhd);
}

// lcdPrintf and lcdPuts don't wan't to cooperate with long strings so here's a small solution
static void displayString(int x, int y, char *str, int len){
	lcdPosition(lcdhd, x, y);
	int i;
	for(i = 0; i < len; i++)
		lcdPutchar(lcdhd, str[i]);

	// if there's still room left on the display, fill the rest of it with spaces
	while(i++ < DISPLAY_LEN)
		lcdPutchar(lcdhd, ' ');
}

void lcdUpdateStatus(int status){
	pthread_mutex_lock(&mutex);

	switch(status){
		case LCD_STATUS_IDLE:
			sprintf(status_str, "Idle");
			break;
		case LCD_STATUS_WATERING:
			sprintf(status_str, "Watering");
			break;
		case LCD_STATUS_MOTION:
			sprintf(status_str, "Motion Detected");
			break;
		default:
			// should never get here. if it does, there's a problem elsewhere
			sprintf(status_str, "INVALID STATUS");
	};

	status_len = strlen(status_str);

	pthread_mutex_unlock(&mutex);
}

void lcdUpdateInfo(double local_temp, double local_humidity,
					double cimis_temp, double cimis_humidity,
					double local_et, double cimis_et,
					double water_savings){

	pthread_mutex_lock(&mutex);

	sprintf(info_str,"Temp:(CIMIS:%.2fF|Local:%.2fF);Humidity:(CIMIS:%.2f%%|Local:%.2f%%);ET:(CIMIS:%.2f|Local:%.2f);Water Savings:%.1f",
			cimis_temp, local_temp, cimis_humidity, local_humidity,
			cimis_et, local_et, water_savings);

	info_len = strlen(info_str);

	pthread_mutex_unlock(&mutex);
}

// thread function to constantly display info from lcdUpdateInfo
void *lcdDisplayInfo(void *args){

	// buffers for what should currently be displayed on the LCD
	char status_buffer[DISPLAY_LEN + 1];
	char info_buffer[DISPLAY_LEN + 1];

	// current position in status string
	int info_pos = 0;

	// counters to know how many times the display has shifted
	// reset display once this reaches a certain number
	int info_counter = 0;

	while(global_run_lcd){
		pthread_mutex_lock(&mutex);

		// copy n characters from strings to buffers
		strncpy(status_buffer, status_str, DISPLAY_LEN);
		strncpy(info_buffer, info_str + info_pos, DISPLAY_LEN);

		// if we're not at the end of either string, increment current position
		if(info_pos < info_len - DISPLAY_LEN)
			info_pos++;

		// once we've scrolled long enough, reset the positions to 0
		if(++info_counter >= info_len - DISPLAY_LEN + 2){
			info_pos = 0;
			info_counter = 0;
		}

		pthread_mutex_unlock(&mutex);

		displayString(0, 0, status_buffer, MIN(status_len, DISPLAY_LEN)); // set cursor position to (0,0) and display status on LCD
		displayString(0, 1, info_buffer, DISPLAY_LEN); // set cursor position to (0,1) and display info on LCD

		delay(500);
	}

	return NULL;
}

#ifdef DEBUG

//main function for testing purpose

int main(void){
    wiringPiSetup();
    double local_temp, local_humidity, cimis_temp, cimis_humidity, local_et, cimis_et, water_savings;
    
    local_temp = 25.20;
    local_humidity = 30.50;
    cimis_temp = 26.20;
    cimis_humidity = 31.50;
    local_et = 10.10;
    cimis_et = 11.10;
    water_savings = 40.45;

	setupLCD();
	lcdUpdateStatus(LCD_STATUS_IDLE);
    lcdUpdateInfo( local_temp, local_humidity, cimis_temp, cimis_humidity, local_et, cimis_et, water_savings);
    lcdDisplayInfo(NULL);
    
    return 0;
}

#endif // DEBUG

