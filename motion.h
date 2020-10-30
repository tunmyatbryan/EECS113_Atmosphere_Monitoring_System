#ifndef MOTION
#define MOTION

#include <wiringPi.h>
#include <stdio.h>

#define sensorPin 7

void setupMotion();

int getMotion();

#endif
