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

#define DISP_LED_GPIO 5 /* phisical pin 25 on rev2 */

static void usage(void);
#define DIN_PIN		1
#define SCLK_PIN	0
#define DC_PIN		2
#define RST_PIN		4
#define CS_PIN		3

/*int contrast = 40;*/ /* lcd contrast  */
static unsigned short dodaemon;
static unsigned short verbose_mode;
static unsigned short led_on_darkness;

typedef enum {LED_UP, LED_DOWN} lstate_t;
/*#define LED_UP 0x1
#define LED_DOWN 0x2*/

typedef struct {
	char	sc_line1_buf[15];
	char	sc_line2_buf[15];
	char	sc_line3_buf[15];
	char	sc_line4_buf[15];
	char	sc_line5_buf[15];
	int	sc_contrast;
	time_t	sc_lastupdate;
} screen_t;

typedef struct {
	int sens_light;
	int sens_uptime;
	int sens_humidity;
} sensor_t;

typedef struct {
	lstate_t	l_state;
	unsigned int	l_pin;
} led_t;

static screen_t *screen_init(const int _din, const int _sclk, const int _dc, const int _rst, const int _cs, const int contrast);
static int	 cur_time(char *, size_t);
static int	 screen_update(screen_t *);
static int	 statistics_update(screen_t *, sensor_t *);
static int	 sensors_read(sensor_t *);
static int	 screen_include(screen_t *);
static void	 led_init(led_t *, unsigned int);
static void	 led_setstate(led_t *, lstate_t);
/*static void	 led_action(lstate_t, int);*/

int 
main (int argc, char **argv)
{
	int ch;
	/*char uptimeInfo[15], lightInfo[15], timebuf[15];*/
	/*int lightlevel;*/
	int ocontrast = 40; /* Contrast defaults to 40 */
	screen_t *sc;
	sensor_t  sensors;
	led_t	  displed;
	extern char *optarg;
	extern int optind;/*, opterr, optopt;*/


	while ((ch = getopt(argc, argv, "lDvc:")) != -1) {
		switch (ch) {
			case 'l':
				led_on_darkness=1;
				break;
			case 'D':
				dodaemon=1;
				break;
			case 'v':
				verbose_mode=1;
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

  


	if ((sc = screen_init(DIN_PIN, SCLK_PIN, DC_PIN, RST_PIN, CS_PIN, ocontrast)) == NULL) {
		syslog(LOG_LOCAL0|LOG_INFO, "failed to init screen");
		err(3, "failed to init screen");
	}

	/* check wiringPi setup */
	delay(2000);
 	if (verbose_mode)
		syslog(LOG_LOCAL0|LOG_DEBUG, "initialisation of screen led");
        led_init(&displed, DISP_LED_GPIO);
 	if (verbose_mode)
		syslog(LOG_LOCAL0|LOG_DEBUG, "initialisation of screen led - bringing up");
	led_setstate(&displed, LED_UP);
  
	for (;;) {
		/* clear lcd */
		LCDclear();
	  
		if (sensors_read(&sensors) < 0)
			syslog(LOG_LOCAL0|LOG_INFO, "failed to read data from sensors");
		if (led_on_darkness) {
			if (sensors.sens_light < 50 && displed.l_state != LED_UP) {
				if (verbose_mode)
					syslog(LOG_LOCAL0|LOG_DEBUG, "bringing screen led up");
				led_setstate(&displed, LED_UP);
			} else if (sensors.sens_light > 50 && displed.l_state == LED_UP)
				led_setstate(&displed, LED_DOWN);
		}

		if (statistics_update(sc, &sensors) < 0)
			syslog(LOG_LOCAL0|LOG_INFO, "failed to update statistics");
		if (screen_include(sc) < 0)
			syslog(LOG_LOCAL0|LOG_INFO, "failed to include file line");
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

static void
led_init(led_t *led, unsigned int pin)
{
	pinMode(pin, OUTPUT);
	led->l_pin = pin;
	delay(2000);
	return;
}

static void led_setstate(led_t *l, lstate_t st)
{
	switch (st) {
		case LED_UP:
			digitalWrite(l->l_pin, HIGH);
			break;
		case LED_DOWN:
			digitalWrite(l->l_pin, LOW);
			break;
	}
	l->l_state = st;
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

static void 
usage(void) {
	fprintf(stderr, "usage: pcd8544 -lDv -c [contrast]\n");
	exit(64);
}
