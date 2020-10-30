#include "cimis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>

#define OUTFILE "cimis_data.csv"


// struct for the file to be recieved over FTP
struct FTPFile{
	const char *filename;
	FILE *stream;
};


// callback functions for FTP transfer through curl
static size_t fwrite_cb(void *buffer, size_t size, size_t n, void *stream){
	struct FTPFile *out = (struct FTPFile *) stream;
	// if the file stream isn't open yet, open it in binary write mode
	if(out->stream == NULL){
		out->stream = fopen(out->filename, "wb");
		if(out->stream == NULL)
			return -1;
	}
	// write to the file stream
	return fwrite(buffer, size, n, out->stream);
}

// gets the (rounded) time and date and puts it in hour and date
// >> Assumes that there's already enough memory allocated to hour and date pointers
static void get_time_date(int *hour, char *date){
	time_t raw_time;
	struct tm *t;
	// get the raw time and convert it to local time
	time(&raw_time);
	t = localtime(&raw_time);

	// convert the hour to a time (sort of)
	// like, hour 13 goes to 1300 hours
	t->tm_hour *= 100;
	// -100 to get the last hour that has completely passed
	t->tm_hour -= 100;
	
	// set hour equal to the last hour
	// back one hour if we're currently in daylight savings
	*hour = (t->tm_isdst) ? t->tm_hour - 100 : t->tm_hour;
	// fill in date with the date string
	// tm_mon ranges from 0 to 11 so you need to add 1
	// tm_year is the number of years since 1900
	sprintf(date, "%d/%d/%d", t->tm_mon + 1, t->tm_mday, t->tm_year + 1900);
}

// finds the most recent data from the CIMIS data file and puts it in the data struct
static int find_most_recent(struct CIMIS_data *data){
	FILE *fp = fopen(OUTFILE, "r");

	if(fp == NULL)
		return CIMIS_DATA_INVALID;

	// used to store the current date and time
	// used to find the correct line in the CIMIS file
	int hour;
	char date[15];

	get_time_date(&hour, date);
	
	char buffer[100];   // this stores the entire line read from the file

	// keep reading lines of the file until EOF
	while(fgets(buffer, 100, fp) != NULL){
		// get the first field and put it into data->station
		char *tok = strtok(buffer, ",");
		data->station = atoi(tok);
		// get the second field and put it into data->date
		tok = strtok(NULL, ",");
		strncpy(data->date, tok, 15);
		// if the read date doesn't match the current date skip this iteration of the loop
		if(strcmp(data->date, date) != 0){
			continue;
		}
		// get the third field and put it into data->hour
		tok = strtok(NULL, ",");
		data->hour = atoi(tok);
		// if the read hour doesn't match the current hour, skip this iteration
		if(data->hour != hour){
			continue;
		}
		// if we get here, then this line is the line that we're looking for
		// get the fifth field and put it into data->et0 and then exit the loop
		// (the fourth field is useless)
		tok = strtok(NULL, ",");
		tok = strtok(NULL, ",");
		data->et0 = atof(tok);
		// get the 13th field and put it into data->air_temp
		for(int i = 0; i < 8; i++) { tok = strtok(NULL, ","); }
		data->air_temp = atof(tok);
		// get the 15th field and put it into data->humidity
		tok = strtok(NULL, ",");
		tok = strtok(NULL, ",");
		data->humidity = atof(tok);
		break;
	}

	if(data->humidity == 0)
		return CIMIS_DATA_INVALID;

	return CIMIS_DATA_VALID;
}

// gets the lastest information from the CIMIS FTP server and puts necessary information into the data struct
// returns CIMIS_DATA_VALID on success
int get_latest_data(struct CIMIS_data *data){
	// variables needed by the curl library
	CURL *curl;
	CURLcode res;
	struct FTPFile ftpfile = {OUTFILE, NULL};
	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	// if initialization succeeded
	if(curl){
		// set the FTP URL
		curl_easy_setopt(curl, CURLOPT_URL, "ftp://ftpcimis.water.ca.gov/pub2/hourly/hourly075.csv");
		// set the callback function for when data needs to be written
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite_cb);
		// give it a pointer to the struct to pass to the callback function
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

		// if you want it to print exactly what it's doing during the transfer
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// do the transfer
		res = curl_easy_perform(curl);

		curl_easy_cleanup(curl);

		// if we failed to get the file, return -1
		if(CURLE_OK != res){
			fprintf(stderr, "Failed to get file. curl error code: %d\n", res);
			return -1;
		}
	}

	// close the file stream if it exists
	if(ftpfile.stream != NULL){
		fclose(ftpfile.stream);
	}

	curl_global_cleanup();

	return find_most_recent(data);
}

#ifdef DEBUG

int main(){
	struct CIMIS_data d;
	if(get_latest_data(&d) == 0){
		printf("%d,%s,%d,%f,%f,%f\n", d.station, d.date, d.hour, d.et0, d.air_temp, d.humidity);
	}
	return 0;
}

#endif // DEBUG
