#include <wiringPi.h>
#include <stdio.h>
#define relayPin 6  //GPIO 25
#define longestWaitTime 60000 // 60000milliseconds or 1 minute
int relaySetup(void){
    #ifdef DDEBUG
      if(wiringPiSetup()==-1){
        printf("setup wiringPi failed.\n");
        return 1;
      }
    #endif
      pinMode(relayPin, OUTPUT);
      return 0;
}
void relayOff(void){
     digitalWrite(relayPin, LOW);
}
static void relayOn(void){
     digitalWrite(relayPin, HIGH);
}
int  relayCheckState(void){
     int reading;
     reading = digitalRead(relayPin);
     return reading;
}
int relayLoop(int detection, int time, int timeStalled){
     if ((detection == 0) || (timeStalled > longestWaitTime)){
          //printf("Sprinkling...\n");
          time--;
          relayOn();
     }
     else if((detection == 1) && (timeStalled <= longestWaitTime)){
          //printf("Movement Detected, Turning OFF\n");
          relayOff();
     }
     return time;
}

#ifdef DEBUG
int main()
{
  int time = 10;
  int count = 10;
  if(relaySetup()==1){
    return 1;
  }
	while(count>0)
	{
    count--;
    printf("Relay State: %d\n", relayCheckState());
		//time = relayLoop(0,time,0);
    if(count<8){
      relayOn();
    }
    else{
      relayOff();
    }
		delay(1000);
	}
  relayOff();
  printf("Relay State: %d\n", relayCheckState());
  printf("Test Program end.\n");
  return 0;
}

#endif
