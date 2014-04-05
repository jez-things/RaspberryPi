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

#include <err.h>
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
/*
static const int _din = 1;
static const int _sclk = 0;
static const int _dc = 2;
static const int _rst = 4;
static const int _cs = 3;
  

int contrast = 40;*/ /* lcd contrast  */
static unsigned short dodaemon;
static unsigned short verbose_mode;

static int cur_time(char *, size_t);
#define LED_UP 0x1
#define LED_DOWN 0x2
static void led_action(int, int);

typedef struct {
	char	sc_line1_buf[15];
	char	sc_line2_buf[15];
	char	sc_line3_buf[15];
	char	sc_line4_buf[15];
	int	sc_contrast;
	time_t	sc_lastupdate;
} screen_t;

typedef struct {
	int sens_light;
	int sens_uptime;
	int sens_humidity;
} sensor_t;

static screen_t *screen_init(const int _din, const int _sclk, const int _dc, const int _rst, const int _cs, const int contrast);
static int screen_update(screen_t *);
static int statistics_update(screen_t *, sensor_t *);
static int sensors_read(sensor_t *);
static int screen_include(screen_t *);

int 
main (int argc, char **argv)
{
	int ch;
	/*char uptimeInfo[15], lightInfo[15], timebuf[15];*/
	/*int lightlevel;*/
	int ocontrast = 40; /* Contrast defaults to 40 */
	screen_t *sc;
	sensor_t  sensors;
	extern char *optarg;
	extern int optind;/*, opterr, optopt;*/


	while ((ch = getopt(argc, argv, "Dvc:")) != -1) {
		switch (ch) {
			case 'D':
				dodaemon++;
				break;
			case 'v':
				verbose_mode++;
				break;
			case 'c':
				ocontrast = atoi(optarg);
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
  
	/*pinMode(DISP_LED_PIN, OUTPUT);*/
	/* check wiringPi setup */
  
	if ((sc = screen_init(1, 0, 2, 4, 3, ocontrast)) == NULL) {
		syslog(LOG_LOCAL0|LOG_INFO, "failed to init screen");
		err(3, "failed to init screen");
	}
	delay(2000);
  
	for (;;) {
		/* clear lcd */
		LCDclear();
	  
		if (sensors_read(&sensors) < 0)
			syslog(LOG_LOCAL0|LOG_INFO, "failed to read data from sensors");
		if (statistics_update(sc, &sensors) < 0)
			syslog(LOG_LOCAL0|LOG_INFO, "failed to update statistics");
		if (screen_include(sc) < 0)
			syslog(LOG_LOCAL0|LOG_INFO, "failed to update statistics");
		/* build screen */
		screen_update(sc);
	  
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
	strftime(timep, timepsiz, "%H:%M", timp);
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


static int 
statistics_update(screen_t *l, sensor_t *sensp)
{
	snprintf(l->sc_line2_buf, sizeof(l->sc_line2_buf), "up %d", sensp->sens_uptime);
	/* light */
	snprintf(l->sc_line3_buf, sizeof(l->sc_line3_buf), "light %d/255", sensp->sens_light );
	return (0);
	  
}

static int
sensors_read(sensor_t *sensp)
{
	struct sysinfo sys_info;

	sensp->sens_light = (int) analogRead(200);

	if(sysinfo(&sys_info) != 0) {
		syslog(LOG_LOCAL0|LOG_ERR, "sysinfo-Error\n");
		return (-1);
	}
	sensp->sens_uptime = sys_info.uptime / 60;/* uptime */
	sensp->sens_humidity = -1;
	return (0);
}

static screen_t *
screen_init(const int _din, const int _sclk, const int _dc, const int _rst, const int _cs, const int contrast)
{
	screen_t *scretp;

	if (wiringPiSetup() == -1) {
		syslog(LOG_LOCAL0|LOG_ERR, "wiringPi-Error");
		return (NULL);
	}
	if (pcf8591Setup(200, 0x48) == -1) {
		syslog(LOG_LOCAL0|LOG_ERR, "wiringPi-Error");
		return (NULL);
  	}
  
	/* init and clear lcd */
	LCDInit(_sclk, _din, _dc, _cs, _rst, contrast);
	LCDclear();
  
	/* show logo */
	LCDshowLogo();

	if ((scretp = calloc(1, sizeof(screen_t))) == NULL) {
		return (NULL);
	}
	scretp->sc_contrast = contrast;
	snprintf(scretp->sc_line1_buf, sizeof(scretp->sc_line1_buf), "Pilab");
	cur_time(scretp->sc_line2_buf, sizeof(scretp->sc_line2_buf));
	return (scretp);
}

static int
screen_include(screen_t *scp)
{
	FILE	*f;
	char	*chp;

	if ((f = fopen("/tmp/pcd8544.line", "r")) == NULL)
		return (-1);
	fgets(scp->sc_line4_buf, sizeof(scp->sc_line4_buf), f);
	fclose(f);
	if ((chp = strchr(scp->sc_line4_buf, '\n')) != NULL)
		*chp = '\0';
	else
		return (-1);
	return (0);
}

static int 
screen_update(screen_t *scp)
{

	LCDdrawstring(0, 0, scp->sc_line1_buf);
	LCDdrawline(0, 10, 83, 10, BLACK);
	if (scp->sc_line2_buf[0] != '\0')
		LCDdrawstring(0, 12, scp->sc_line2_buf);
	if (scp->sc_line3_buf[0] != '\0')
		LCDdrawstring(0, 20, scp->sc_line3_buf);
	if (scp->sc_line4_buf[0] != '\0')
		LCDdrawstring(0, 28, scp->sc_line4_buf);
	LCDdisplay();
	return (0);
}
