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


#include "powermate.h"
#include <linux/input.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Uncoment the next line if you want to compile using -ansi */
/* #include <linux/limits.h> */


struct pm_event {
	long a;
	long b;
	short type;
	short code;
	unsigned int data;
};


/* To be implemented in a future version */
/*
unsigned char check_powermate_support(void)
{
}
*/


int search_powermate_devices(char *directory, char ***found_devices)
{
	int fd, count = 0;
	size_t size;
	DIR* dir;
	struct stat entry_stat;
	char *default_directory = "/dev/input", **a;
	union {	struct dirent entry;
              	char b[offsetof(struct dirent, d_name) + NAME_MAX + 1];
	} u;
	struct dirent *result = &u.entry;
	char *path, *offset;

	if (directory == NULL) directory = default_directory;
	if ((dir = opendir(directory)) == NULL) return -1;

	/* To allocate a new buffer for a complete path is better
	than chdir(). If another thread tries to access to the
	filesystem and it spects to find the previous current
	directory can get a surprise... */

	if ((path = (char *)malloc((size = strlen(directory)) + NAME_MAX)) == NULL)
		return -1;

	strcpy(path, directory);
	offset = path + size;

	if (directory[size - 1] != '/') {
		path[size] = '/';
		offset++;
	}

	*found_devices = NULL;

	for (	readdir_r(dir, &u.entry, &result);
		result == &u.entry;
		readdir_r(dir, &u.entry, &result)
	) {
		strcpy(offset, u.entry.d_name);

		if (	stat(path, &entry_stat) == -1
			|| !S_ISCHR(entry_stat.st_mode)
			|| (fd = open(path, O_RDONLY)) == -1
		) continue;

		if (get_powermate_model(fd) != NULL) {
			size = (count + 1) * sizeof(char *);

			if ((a = (char **)realloc((void *)*found_devices, size)) == NULL)
				return count;

			/* (char *) cast to avoid warning when compiling with -ansi gcc option */
			(*found_devices = a)[count] = (char *)strdup((const char *)path);
			count++;
		}

		close(fd);
	}

	free(path);
	closedir(dir);
	return count;
}


const char *get_powermate_model(int fd)
{
	unsigned int index;
	char id_buff[256];
	const char *id_strings[] = {
		"Griffin SoundKnob",
		"Griffin PowerMate"
	};

	ioctl(fd, EVIOCGNAME(255), &id_buff);
	for (index = 0; index != 2; index ++)
		if (!strcmp(id_strings[index], id_buff)) return id_strings[index];
	return NULL;
}


PowerMate *powermate_new(const char *device, PowerMateHandlers *handlers)
{
	PowerMate *pm = (PowerMate *)malloc(sizeof(PowerMate));

	if (pm == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	if (stat(device, &pm->stat)) goto failed;
	if (!S_ISCHR(pm->stat.st_mode)) goto failed_no_device;
	if ((pm->input = open(device, O_RDONLY)) == -1) goto failed;
	if ((pm->model_id = get_powermate_model(pm->input)) == NULL) goto failed_no_device;
	pm->output = open(device, O_WRONLY);
	if (handlers != NULL) powermate_set_handlers(pm, handlers);
	/* (char *) cast to avoid warning when compiling with -ansi gcc option */
	if ((pm->device = (char *)strdup(device)) == NULL) goto failed;
	return pm;

	failed_no_device:
		errno = ENODEV;
	failed:
		free(pm);
		return NULL;
}


int powermate_destroy(PowerMate *pm)
{
	if (pm->input > -1) close(pm->input);
	if (pm->output > -1) close(pm->output);
	free(pm);
	return 0;
}


/*	Led configuration format:

					Host specific integer value
	 __ __ __ __ __ ___ ____ _____ _____________|________________________________________________
												      \
				.. .. .. 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
					 \/ \/ \___/  \_______________________/ \_____________________/
					 |  |	 |		  |			   |
	pulse when host is power on -----+  |	 |		  |			   |
	pulse when host is sleeping --------+	 |		  |			   |
	pulse mode (0, 1 or 2) ------------------+		  |			   |
	pulse speed (0 / 510, >= 286 freezes rotation events) ----+			   |
	static brightness (0 / 255) -------------------------------------------------------+

	Comments from <Linux Kernel Sources>/drivers/usb/input/powermate.c:

	the powermate takes an operation and an argument for its pulse algorithm.
	the operation can be:
	0: divide the speed
	1: pulse at normal speed
	2: multiply the speed
	the argument only has an effect for operations 0 and 2, and ranges between
	1 (least effect) to 255 (maximum effect).

	thus, several states are equivalent and are coalesced into one state.

	we map this onto a range from 0 to 510, with:
	0 -- 254    -- use divide (0 = slowest)
	255         -- use normal speed
	256 -- 510  -- use multiple (510 = fastest).

	Only values of 'arg' quite close to 255 are particularly useful/spectacular.			*/


int powermate_get_events(PowerMate *pm)
{
	struct pm_event event;
	struct timeval tv;
	unsigned long long int
		last_up = 0, last_down = 0,
		last_left = 0, last_right = 0,
		last_led = 0, now;
	int retval;

	while (read(pm->input, &event, sizeof(struct pm_event)) != -1) {
		if (gettimeofday(&tv, NULL) == -1) return -1;
		now = tv.tv_sec * 1000 + tv.tv_usec / 1000;

		switch (event.type) {
			case EV_KEY:
				if (event.data == 0) {
					if (pm->handlers.up != NULL) {
						if ((retval = pm->handlers.up(
							pm, pm->handlers.data,
							last_up ? now - last_up : 0
						))) return retval;
						last_up = now;
						}
				} else if (event.data == 1) {
					if (pm->handlers.down != NULL) {
						if ((retval = pm->handlers.down(
							pm, pm->handlers.data,
							last_down ? now - last_down : 0
						))) return retval;
						last_down = now;
					}
				}
				break;

			case EV_REL:
				if ((int)event.data > 0) {
					if (pm->handlers.right != NULL) {
						if ((retval = pm->handlers.right(
							pm, pm->handlers.data,
							last_right ? now - last_right : 0,
							event.data
						))) return retval;
						last_right = now;
					}
				} else if ((int)event.data < 0) {
					if (pm->handlers.left != NULL) {
						if ((retval = pm->handlers.left(
							pm, pm->handlers.data,
							last_left ? now - last_left : 0,
							(unsigned int)-(int)event.data
						))) return retval;
						last_left = now;
					}
				}
				break;

			case EV_MSC:
				pm->led.static_brightness = (unsigned char)(event.data & 0xFF);
				pm->led.pulse_speed = (unsigned short)(event.data >> 8) & 0x1FF;
				pm->led.pulse_table = (unsigned char)(event.data >> 17) & 3;
				pm->led.pulse_asleep = (unsigned char)(event.data >> 19) & 1;
				pm->led.pulse_awake = (unsigned char)(event.data >> 20) & 1;

				if (pm->handlers.led != NULL)
					if ((retval = pm->handlers.led(
						pm, pm->handlers.data,
						last_led ? now - last_led : 0,
						&pm->led
					))) return retval;
					last_led = now;
				break;
		}
	}

	return -1;
}


int powermate_set_handlers(PowerMate *pm, PowerMateHandlers *handlers)
{
	if (handlers == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (handlers != &pm->handlers) memmove(&pm->handlers, handlers, sizeof(PowerMateHandlers));
	return 0;
}


int powermate_set_led(PowerMate *pm, PowerMateLED *led)
{
	if (led == NULL) led = &pm->led;

	if (pm->output < 0) {
		errno = EBADF;
		return -1;
	}

	if (	led->pulse_table > 2 || led->pulse_asleep > 1 ||
		led->pulse_awake > 1 || led->pulse_speed > 510
	) {
		errno = EINVAL;
		return -1;
	} else {struct pm_event e = {
			0, 0, EV_MSC, MSC_PULSELED,
			((unsigned int)led->static_brightness & 0xFF)
			| ((unsigned int)(led->pulse_speed & 0x1FF) << 8)
			| ((unsigned int)(led->pulse_table) << 17)
			| ((unsigned int)(led->pulse_asleep) << 19)
			| ((unsigned int)(led->pulse_awake) << 20)
		};

		if (write(pm->output, &e, sizeof(struct pm_event)) < 0) return -1;
		if (led != &pm->led)
			memmove(&pm->led, led, sizeof(PowerMateLED));
		return 0;
	}
}


int powermate_set_static_brightness(PowerMate *pm, unsigned char brightness)
{
	pm->led.static_brightness = brightness;
	return powermate_set_led(pm, NULL);
}


int powermate_set_pulse_speed(PowerMate *pm, unsigned short speed)
{
	pm->led.pulse_speed = speed;
	return powermate_set_led(pm, NULL);
}


int powermate_set_pulse_table(PowerMate *pm, unsigned char table)
{
	pm->led.pulse_table = table;
	return powermate_set_led(pm, NULL);
}


int powermate_set_pulse_asleep(PowerMate *pm, unsigned char state)
{
	pm->led.pulse_asleep = state;
	return powermate_set_led(pm, NULL);
}


int powermate_set_pulse_awake(PowerMate *pm, unsigned char state)
{
	pm->led.pulse_awake = state;
	return powermate_set_led(pm, NULL);
}


int powermate_set_all(
	PowerMate *pm,
	unsigned char static_brightness,
	unsigned short pulse_speed,
	unsigned char pulse_table,
	unsigned char pulse_asleep,
	unsigned char pulse_awake
){
	pm->led.static_brightness = static_brightness;
	pm->led.pulse_speed = pulse_speed;
	pm->led.pulse_table = pulse_table;
	pm->led.pulse_asleep = pulse_asleep;
	pm->led.pulse_awake = pulse_awake;
	return powermate_set_led(pm, NULL);
}


/* libpowermate.c EOF */
