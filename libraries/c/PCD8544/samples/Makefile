CFLAGS=-W -Wall -ansi -pedantic -g
pcd8544daemon: pcd8544_daemon.o PCD8544.o ../PCD8544.h
	cc $(CFLAGS) -lwiringPi -o pcd8544daemon pcd8544_daemon.o PCD8544.o

pcd8544_daemon.o: pcd8544_daemon.c ../PCD8544.h
	cc $(CFLAGS) -I.. -c -o pcd8544_daemon.o pcd8544_daemon.c

PCD8544.o: ../PCD8544.c
	cc $(CFLAGS) -I.. -c -o PCD8544.o ../PCD8544.c


clean:
	rm -v *.o pcd8544daemon
