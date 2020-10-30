#ifndef RELAY_H
#define RELAY_H

#define relayPin 6 //GPIO Pin = 25
#define longestWaitTime 60000 // 60000 milliseconds or 1 minute

int relaySetup(void);
void relayOff(void);
int relayCheckState(void);
int relayLoop(int detection, int time, int timeStalled);


#endif // RELAY_H
