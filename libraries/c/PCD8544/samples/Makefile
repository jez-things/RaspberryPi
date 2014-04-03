
pcd8544test: pcd8544_rpi.o PCD8544.o ../PCD8544.h
	cc -lwiringPi -o pcd8544test pcd8544_rpi.o PCD8544.o

pcd8544_rpi.o: pcd8544_rpi.c ../PCD8544.h
	cc -I.. -c -o pcd8544_rpi.o pcd8544_rpi.c

PCD8544.o: ../PCD8544.c
	cc -I.. -c -o PCD8544.o ../PCD8544.c


clean:
	rm -v *.o
