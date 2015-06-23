/*
	powermate-test v1.0
	Simple Griffin PowerMate test utility based in libpowermate.

	Copyright(C) 2004, 2005 Manuel Sainz de Baranda Goñi
	Released under the terms of the GNU General Public License version 2

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <powermate.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termio.h>

#define VERSION "1.0"
#define DEFAULT_POWERMATE_DEVICE_DIR "/dev/input"

enum {	STATIC_INSENSITIVE,
	PULSE_INSENSITIVE,
	STATIC_SENSITIVE,
	PULSE_SENSITIVE
};

time_t press_time;
unsigned int mode;
PowerMateLED led = {255, 255, 1, 0, 0};
char unerror[] = "error: unexpected error, errno = %d (%s)\n";


int on_left(PowerMate *pm, void *data, unsigned long long int tesle, unsigned int units)
{
	if (mode > PULSE_INSENSITIVE && pm->output != -1) {
		if (mode == STATIC_SENSITIVE) led.static_brightness =
			led.static_brightness > units ? led.static_brightness - units : 0;

		else led.pulse_speed =
			led.pulse_speed > units ? led.pulse_speed - units : 0;

		if (powermate_set_led(pm, &led) == -1) {
			printf(unerror, errno, strerror(errno));
			return 1;
		}
	}

	printf("rotated left %d units, last time was %lu millisecons ago\n", units, tesle);
	return 0;
}


int on_right(PowerMate *pm, void *data, unsigned long long int tesle, unsigned int units)
{
	if (mode > PULSE_INSENSITIVE && pm->output != -1) {
		if (mode == STATIC_SENSITIVE) led.static_brightness =
			units + led.static_brightness < 255 ? led.static_brightness + units : 255;

		else led.pulse_speed =
			units + led.pulse_speed < 510 ? led.pulse_speed + units : 510;

		if (powermate_set_led(pm, &led) == -1) {
			printf(unerror, errno, strerror(errno));
			return 1;
		}
	}

	printf("rotated right %d units, last time was %lu milliseconds ago\n", units, tesle);
	return 0;
}


int on_down(PowerMate *pm, void *data, unsigned long long int tesle)
{
	press_time = time(NULL);
	printf("button pressed, last time was %lu milliseconds ago\n", tesle);
	return 0;
}


int on_up(PowerMate *pm, void *data, unsigned long long int tesle)
{
	if (time(NULL) - press_time >= 3) return 1;
	press_time = 0;
	printf("button released, last time was %lu milliseconds ago\n", tesle);
	return 0;
}


int on_led(PowerMate *pm, void *data, unsigned long long int tesle, PowerMateLED *l)
{
	printf(	"%s: LED set {%hhu, %hu, %hhu, %hhu, %hhu}\n",
		pm->device,
		l->static_brightness,
		l->pulse_speed,
		l->pulse_table,
		l->pulse_asleep,
		l->pulse_awake);

	return 0;
}


int main(int argc, char **argv)
{
	char **devices;
	PowerMate *pm;
	PowerMateHandlers handlers = {on_left, on_right, on_down, on_up, on_led, NULL};
	int i, count;
	int option;

	printf( "powermate-test v" VERSION "\n"
		"Copyright(C) 2004, 2005 Manuel Sainz de Baranda\n"
		"Released under the terms of the GNU General Public License v2\n\n"
		"Scanning " DEFAULT_POWERMATE_DEVICE_DIR "..."
	);
	count = search_powermate_devices(NULL, &devices);

	if (!count) {
		printf(" error: no PowerMate device found!\n");
		return ENODEV;
	}

	printf(" done!, %d devices found:\n", count);
	for (i = 0; i < count; printf("#%d: %s\n", i, devices[i++]));
	if (count > 1) while (count) free(devices[--count]);
	printf("Initializing %s...", *devices);

	if ((pm = powermate_new(*devices, &handlers)) == NULL) {
		printf(	" error: can not create PowerMate object: errno = %d (%s)",
			errno,
			strerror(errno)
		);
		goto error;
	}

	press_time = 0;

	if (pm->output != -1) {
		struct termio iop;

		puts(	" ok, read and write allowed\n"
			"Setting initial LED values to:\n"
			"static_brightness = 255\n"
			"pulse_speed       = 255\n"
			"pulse_mode        = 1\n"
			"pulse_awake       = 0\n"
			"pulse_asleep      = 0"
		);

		do {	printf(	"\nPlease, select LED mode:\n"
				" a - Static brightness.\n"
				" b - Use the knob to increase/decrease static brightness.\n"
				" c - Uniform pulse.\n"
				" d - Use the knob to increase/decrease pulse speed.\n"
				" e - Exit.\n"
				"(Type a, b, c, d or e, default a) > "
			);

			switch (getchar()) {
				case 'a':
					mode = STATIC_INSENSITIVE;
					led.static_brightness = 255;
					led.pulse_awake = POWERMATE_PULSE_STATE_OFF;
					break;

				case 'b':
					mode = STATIC_SENSITIVE;
					led.static_brightness = 127;
					led.pulse_awake = POWERMATE_PULSE_STATE_OFF;
					break;

				case 'c':
					mode = PULSE_INSENSITIVE;
					led.pulse_speed = 255;
					led.pulse_awake = POWERMATE_PULSE_STATE_ON;
					break;

				case 'd':
					mode = PULSE_SENSITIVE;
					led.pulse_speed = 255;
					led.pulse_awake = POWERMATE_PULSE_STATE_ON;
					break;

				case 'e':
					goto end;

				default:
					continue;
			}

			if (powermate_set_led(pm, &led) == -1) {
				printf(unerror, errno, strerror(errno));
				goto error;
			}

			while ((option = getchar()) != '\n' && option != EOF);
		} while (powermate_get_events(pm) != -1);

	} else {
		puts(" warning!!, only read is allowed\nLED demonstrations dissabled.");
		powermate_get_events(pm);
	}

	printf("%s desconectado!\n", pm->model_id);

	end:
	free(*devices);
	free(devices);
	return 0;

	error:
	free(*devices);
	free(devices);
	return errno;
}
