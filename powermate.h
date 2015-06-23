/*
	libpowermate v1.0
	A very tiny library for Griffin Powermate control and handling.

	Copyright(C) 2004, 2005 Manuel Sainz de Baranda Goñi
	Distributed under the terms of the GNU Lesser General Public License version 2.1

	This file is part of libpowermate.

	libpowermate is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef __POWERMATE_H__
#define __POWERMATE_H__

#include <sys/stat.h>
#include <sys/time.h>

typedef enum {
	POWERMATE_PULSE_MODE_DIVIDE,
	POWERMATE_PULSE_MODE_NORMAL,
	POWERMATE_PULSE_MODE_MULTIPLY
} PowerMatePulseMode;

typedef enum {
	POWERMATE_PULSE_STATE_OFF,
	POWERMATE_PULSE_STATE_ON
} PowerMatePulseState;

enum {	POWERMATE_PULSE_ASLEEP,
	POWERMATE_PULSE_AWAKE
};

struct PowerMate;
typedef struct PowerMate PowerMate;

typedef struct {
	unsigned char static_brightness; /* LED brightness */
	unsigned short int pulse_speed;	 /* pulsing speed modifier (0 ... 510);
					    best results around 255, > 285 causes event reception malfunction */
	unsigned char pulse_table;	 /* pulse table (0, 1, 2 valid) */
	unsigned char pulse_asleep;	 /* pulse while asleep? */
	unsigned char pulse_awake;	 /* pulse constantly? */
} PowerMateLED;


typedef int	(*PowerMateRotateFunc)	(PowerMate* pm,
					void* data,
					unsigned long long int tesle,
					unsigned int units);
typedef int	(*PowerMateButtonFunc)	(PowerMate *pm,
					void *data,
					unsigned long long int tesle);
typedef int	(*PowerMateLEDFunc)	(PowerMate *pm,
					void *data,
					unsigned long long int tesle,
					PowerMateLED *led);

typedef struct {
	PowerMateRotateFunc left;
	PowerMateRotateFunc right;
	PowerMateButtonFunc down;
	PowerMateButtonFunc up;
	PowerMateLEDFunc led;
	void *data;
} PowerMateHandlers;

struct PowerMate {
	int input;
	int output;
	char *device;
	struct stat stat;
	const char *model_id;
	PowerMateLED led;
	PowerMateHandlers handlers;
};


#define powermate_get_state(p) p->state

const char*	get_powermate_model		(int fd);
PowerMate*	powermate_new			(const char *device,
						PowerMateHandlers *handlers);
int		powermate_destroy		(PowerMate *pm);
int		powermate_get_events		(PowerMate *pm);
int		powermate_set_handlers		(PowerMate *pm,
						PowerMateHandlers *handlers);
int		powermate_set_led		(PowerMate *pm,
						PowerMateLED *led);
int		powermate_set_static_brightness	(PowerMate *pm,
						unsigned char brightness);
int		powermate_set_pulse_speed	(PowerMate *pm,
						unsigned short speed);
int		powermate_set_pulse_table	(PowerMate *pm,
						unsigned char table);
int		powermate_set_pulse_asleep	(PowerMate *pm,
						unsigned char state);
int		powermate_set_pulse_awake	(PowerMate *pm,
						unsigned char state);
int		powermate_set_all		(PowerMate *pm,
						unsigned char static_brightness,
						unsigned short pulse_speed,
						unsigned char pulse_table,
						unsigned char pulse_asleep,
						unsigned char pulse_awake);

#endif /* __POWERMATE_H__ */
