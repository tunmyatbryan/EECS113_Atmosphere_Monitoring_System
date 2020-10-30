EXEC=program

CC=g++
CFLAGS=-Wall -c
LFLAGS=-lcurl -lpthread -lwiringPi -lwiringPiDev

%.o: %.cpp
	$(CC) -o $@ $< $(CFLAGS)

%.o: %.c
	$(CC) -o $@ $< $(CFLAGS)

${EXEC}: main.o cimis.o DHT.o motion.o relay.o LCD.o
	$(CC) -o $@ $^ $(LFLAGS)

cimis_d: CFLAGS+=-g -DDEBUG
cimis_d: cimis.o
	$(CC) -o $@ $^ $(LFLAGS)

LCD: CFLAGS+=-g -DDEBUG
LCD: LCD.o
	$(CC) -o $@ $^ $(LFLAGS)

dht_d: CFLAGS+=-g -DDEBUG
dht_d: DHT.o
	$(CC) -o $@ $^ $(LFLAGS)

motion_d: CFLAGS+=-g -DDEBUG
motion_d: motion.o
	$(CC) -o $@ $^ $(LFLAGS)

relay_d: CFLAGS+=-g -DDEBUG
relay_d: relay.o
	$(CC) -o $@ $^ $(LFLAGS)

clean:
	rm -f ./${EXEC}
	rm -f ./cimis_d
	rm -f ./relay_d
	rm -f ./LCD
	rm -f ./dht_d
	rm -f ./motion_d
	rm -f ./*.o
