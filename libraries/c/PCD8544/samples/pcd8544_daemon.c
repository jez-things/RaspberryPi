/*
=================================================================================
 Name        : pcd8544_daemon.c
 Version     : 0.1

 Copyright (C) 2012 by Andre Wussow, 2012, desk@binerry.de
 Copyright (C) 2014 by Jez Orkowski, 2014, jez@obin.org

 Description :
     A simple PCD8544 LCD (Nokia3310/5110) for Raspberry Pi for displaying some system informations.
	 Makes use of WiringPI-library of Gordon Henderson (https://projects.drogon.net/raspberry-pi/wiringpi/)

	 Recommended connection (http://www.raspberrypi.org/archives/384):
	 LCD pins      Raspberry Pi
	 LCD1 - GND    P06  - GND
	 LCD2 - VCC    P01 - 3.3V
	 LCD3 - CLK    P11 - GPIO0
	 LCD4 - Din    P12 - GPIO1
	 LCD5 - D/C    P13 - GPIO2
	 LCD6 - CS     P15 - GPIO3
	 LCD7 - RST    P16 - GPIO4
	 LCD8 - LED    P01 - 3.3V 

================================================================================
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
================================================================================
 */
#include <wiringPi.h>
#include <pcf8591.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include "PCD8544.h"

#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define DISP_LED_PIN 24 /* phisical pin 25 on rev2 */

static void 
usage(void) {
	fprintf(stderr, "usage: pcd8544 -Dv -c [contrast");
	exit(64);

}

/* pin setup */
static const int _din = 1;
static const int _sclk = 0;
static const int _dc = 2;
static const int _rst = 4;
static const int _cs = 3;
  

int contrast = 40;/* lcd contrast  */
static unsigned short dodaemon;
static unsigned short verbose_mode;

static int cur_time(char *, size_t);
#define LED_UP 0x1
#define LED_DOWN 0x2
static void led_action(int, int);

int 
main (int argc, char **argv)
{
	int ch;
	unsigned long uptime;
	struct sysinfo sys_info;
	char uptimeInfo[15], lightInfo[15], timebuf[15];
	int lightlevel;
	extern char *optarg;
	extern int optind, opterr, optopt;


	while ((ch = getopt(argc, argv, "Dvc:")) != -1) {
		switch (ch) {
			case 'D':
				dodaemon++;
				break;
			case 'v':
				verbose_mode++;
				break;
			case 'c':
				contrast = atoi(optarg);
				break;
			default:
				usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (dodaemon) {
		daemon(1, 0);
	}
	openlog("pcd8544", LOG_PID|LOG_PERROR, LOG_LOCAL0);
	/* print infos */
	syslog(LOG_LOCAL0|LOG_INFO, "pilab");
  
	pinMode(DISP_LED_PIN, OUTPUT);
	/* check wiringPi setup */
	if (wiringPiSetup() == -1) {
		syslog(LOG_LOCAL0|LOG_ERR, "wiringPi-Error");
		exit(1);
	}
	if (pcf8591Setup(200, 0x48) == -1) {
		syslog(LOG_LOCAL0|LOG_ERR, "wiringPi-Error");
		exit(1);
  	}
  
	/* init and clear lcd */
	LCDInit(_sclk, _din, _dc, _cs, _rst, contrast);
	LCDclear();
  
	/* show logo */
	LCDshowLogo();
  
	delay(2000);
  
	for (;;) {
		/* clear lcd */
		LCDclear();
	  
		if(sysinfo(&sys_info) != 0) {
			syslog(LOG_LOCAL0|LOG_ERR, "sysinfo-Error\n");
		}
	  
		/* uptime */
		cur_time(timebuf, sizeof timebuf);
		uptime = sys_info.uptime / 60;
		sprintf(uptimeInfo, "up %ld", uptime);
		/* light */
		lightlevel = (int) analogRead(200);
		sprintf(lightInfo, "light %ld/255", lightlevel);
		if (lightlevel < 50) {
			led_action(DISP_LED_PIN, LED_UP);
		}
	  
	  
		/* build screen */
		LCDdrawstring(0, 0, "PiLab:");
		LCDdrawline(0, 10, 83, 10, BLACK);
		LCDdrawstring(0, 12, uptimeInfo);
		LCDdrawstring(0, 20, timebuf);
		LCDdrawstring(0, 28, lightInfo);
		LCDdisplay();
	  
		sleep(5);
	}
  return 0;
}

static int 
cur_time(char *timep, size_t timepsiz)
{
	time_t curtime;
	struct tm *timp;

	curtime = time(NULL);
	timp = localtime(&curtime);
	strftime(timep, timepsiz, "%T", timp);
	return (0);
}

static void led_action(int action, int pin)
{
	switch (action) {
		case LED_UP:
			digitalWrite(pin, HIGH);
			break;
		case LED_DOWN:
			digitalWrite(pin, LOW);
			break;
	}
	return;
}
