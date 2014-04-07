CFLAGS=-W -Wall -ansi -pedantic -g  -D_BSD_SOURCE 
PROG=pcd8544d
$(PROG): pcd8544_daemon.o PCD8544.o ../PCD8544.h
	cc $(CFLAGS) -lwiringPi -o $(PROG)  pcd8544_daemon.o PCD8544.o

pcd8544_daemon.o: pcd8544_daemon.c ../PCD8544.h
	cc $(CFLAGS) -I.. -c -o pcd8544_daemon.o pcd8544_daemon.c

PCD8544.o: ../PCD8544.c
	cc $(CFLAGS) -I.. -c -o PCD8544.o ../PCD8544.c


clean:
	rm -fv *.o pcd8544d 
	pkill pcd8544d 

run: pcd8544d
	@echo "Running $<"
	@./$< -D -v -c 40 -l
