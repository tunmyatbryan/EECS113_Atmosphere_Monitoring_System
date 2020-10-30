#include <stdio.h>
#include <wiringPi.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "cimis.h"
#include "DHT.hpp"
#include "relay.h"
#include "motion.h"
#include "LCD.h"


#define AREA           200 // square feet
#define WATER_RATE    1020 // gallons per hour
#define IE            0.75
#define PF             0.5

// in gallons per day
#define WATER_AMT(et) ((et)*PF*AREA*0.62/IE)

// input seconds, gives milliseconds
#define SECONDS(x) ((x) * 1000)

#define WAIT_TIME 60000 //milliseconds

#define LOG_FILE "temp_log.csv"

// global flag for when the lcd display thread should stop
int global_run_lcd;

// the most recent local ET and cimis data
double avg_temp;
double avg_humidity;
double et_local;
struct CIMIS_data cimis_data;

double water_saved;

// gets set to 0 on Ctrl-C
int running;

// signal handler for SIGINT (i.e. Ctrl-C)
void handle_sigint(int sig) { running = 0; }

void printLog(double savings, double water_time);

// gets data from CIMIS and updates the most recent data (from above)
void hourlyCheck(double temp, double humidity);
void waterPlants();

static inline double C_to_F(double temp_C) { return (temp_C * (9.0 / 5.0)) + 32.0; }

int main(){
	if(wiringPiSetup() == -1){
		fprintf(stderr, "wiringPi setup failed,\n");
		return -1;
	}

	/*** Call whatever additional setup is needed here ***/
	printf("Initializing devices.\n");
	relaySetup();
	setupMotion();
	setupLCD();

	water_saved = 0.0l;

	DHT dht;

	avg_temp = 0.0l;
	avg_humidity = 0.0l;

	// counts between 0 and 59, gets incremented eveytime local temp + humidity is checked
	// used to track when an hour has passed
	int counter = 59; // starts at 59 so the first iteration of the loop sets it to 0

	// so that the lcd display thread can actually run
	global_run_lcd = 1;

	printf("Starting LCD.\n");
	// start lcd display thread
	pthread_t lcd_tid;
	pthread_create(&lcd_tid, NULL, lcdDisplayInfo, NULL);

	lcdUpdateStatus(LCD_STATUS_IDLE);

	// call handle_sigint on SIGINT signal
	signal(SIGINT, handle_sigint);

	// counter so we know what to divide by to get average
	int local_count = 0;

	FILE *fp = fopen(LOG_FILE, "w");
	if(fp != NULL){
		fprintf(fp, "Time(PT),Local Temp,Local Humidity,Local ET,CIMIS Temp,CIMIS Humidity,CIMIS ET,Water Time(seconds),Water Saved(Gallons)\n");
		fclose(fp);
		fp = NULL;
	}

	printf("Entering main loop.\n");
	running = 1;
	while(running){
		counter = (counter + 1) % 60;

		// read temp+humidity every 2 seconds until we get a good reading
		while(dht.readDHT11(DHT_PIN) != DHTLIB_OK)
			delay(2000);

		printf("Local Temperature: %.2f F\t Humidity: %.2f%%\n", C_to_F(dht.temperature), dht.humidity);
		// accumulate temp and humidity readings over the hour
		avg_temp += dht.temperature;
		avg_humidity += dht.humidity;

		local_count++;

		// after one hour, do this
		if(counter == 0){
			avg_temp /= (double) local_count;
			avg_humidity  /= (double) local_count;

			printf("Getting CIMIS data.\n");
			// update cimis data and local ET
			hourlyCheck(C_to_F(avg_temp), avg_humidity);
			printf("CIMIS Temperature: %.2f F\t Humidity: %.2f%%\n", cimis_data.air_temp, cimis_data.humidity);

			// give the LCD all the new data
			lcdUpdateInfo(C_to_F(dht.temperature), dht.humidity,
					cimis_data.air_temp, cimis_data.humidity,
					et_local, cimis_data.et0, water_saved);

			waterPlants();

			// reset temp and humidity accumulators
			avg_temp = 0.0l;
			avg_humidity = 0.0l;
			local_count = 0;
		}
		else{
			// give LCD new data (namely, local temp and humidity)
			lcdUpdateInfo(C_to_F(dht.temperature), dht.humidity,
					cimis_data.air_temp, cimis_data.humidity,
					et_local, cimis_data.et0, water_saved);
		}

		delay(WAIT_TIME); // 1 minute delay
	}

	printf("\nExiting...\n");

	global_run_lcd = 0;
	pthread_join(lcd_tid, NULL);
	cleanupLCD();
	relayOff();

	return 0;
}

void hourlyCheck(double temp, double humidity){
	// get CIMIS data once a minute until we get the information we need
	while(get_latest_data(&cimis_data) != CIMIS_DATA_VALID){
		delay(60000);
	}

	double et = cimis_data.et0 * (temp / cimis_data.air_temp) * (cimis_data.humidity / humidity);
	et_local = et;
}

void waterPlants(){
	double amount = WATER_AMT(et_local * 24); // gallons per day
	int water_time = SECONDS((3600 * amount) / (24 * WATER_RATE));
	int tmp; // used to store return value from relayLoop
	int detect;
	int timeStalled = 0;
	
	// calculation is (gallons per day) / (24 hours) to get gallons of water used for this hour
	double saved_this_hour = ((WATER_AMT(cimis_data.et0 * 24) - amount) / 24);
	water_saved += saved_this_hour;
	// water_time is in ms so divide by 1000 for seconds
	printLog(saved_this_hour, (double)water_time / 1000);

	printf("Watering plants for %.2f seconds.\n", (float) water_time / 1000.0);

	lcdUpdateStatus(LCD_STATUS_WATERING);

	// water_time is the remaining number of milliseconds we need to water for
	while(water_time > 0 && running){
		detect = getMotion();
		// relayLoop returns the remaining time after it's done with what it's doing
		tmp = relayLoop(detect, water_time, timeStalled);
		if(water_time == tmp){
			lcdUpdateStatus(LCD_STATUS_MOTION);
			timeStalled++;
		}
		else{
			lcdUpdateStatus(LCD_STATUS_WATERING);
		}
		water_time = tmp;
		delay(1); // wait a millisecond
	}
	relayOff();

	printf("Done watering.\n");

	lcdUpdateStatus(LCD_STATUS_IDLE);
}

void printLog(double savings, double water_time){
	time_t raw_time;
	struct tm *t;
	time(&raw_time);
	t = localtime(&raw_time);

	FILE *fp = fopen(LOG_FILE, "a");
	if(fp != NULL){
		fprintf(fp, "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
				(t->tm_hour * 100) + t->tm_min,
				C_to_F(avg_temp),
				avg_humidity,
				et_local,
				cimis_data.air_temp,
				cimis_data.humidity,
				cimis_data.et0,
				water_time,
				savings);
		fclose(fp);
	}
}

