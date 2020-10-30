#include "motion.h"

void setupMotion(){
	pinMode(sensorPin, INPUT);
}

int getMotion()
{
	int motion;

	if(digitalRead(sensorPin) == HIGH)
	{
		motion = 1;
	} else {
		motion = 0;
	}

	return motion;
}

#ifdef DEBUG
int main()
{
	int motion;
	setupMotion();
	while(1)
	{
		motion = getMotion();
		printf("motion: %d\n", motion);
	}
}
#endif
