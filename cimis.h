#ifndef CIMIS_H
#define CIMIS_H

#define CIMIS_DATA_VALID   0
#define CIMIS_DATA_INVALID 1

// stores the data we need from CIMIS
struct CIMIS_data{
	int station;
	char date[15];
	int hour; // NOTE: this is in PST (not PDT)
	double et0;
	double air_temp; // in Fahrenheit
	double humidity; // in percent (not decimal)
};

// gets the lastest information from the CIMIS FTP server and puts necessary information into the data struct
// returns CIMIS_DATA_VALID on success
int get_latest_data(struct CIMIS_data *data);


#endif // CIMIS_H
