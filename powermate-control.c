/*
	powermate-control v1.0
	Simple Griffin PowerMate control utility based in libpowermate.

	Copyright(C) 2004, 2005 Manuel Sainz de Baranda y Goñi
	Distributed under the terms of the GNU General Public License version 2

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
#include <string.h>
#include <errno.h>

#define VERSION "1.0"
#define COMMANDS 10

enum {	BRIGHTNESS,
	MODE,
	SPEED,
	PULSE_AWAKE,
	PULSE_ASLEEP
};


int get_integer(char *string)
{
	char *s = string;

	while (*s) if (*s < '0' || *(s++) > '9') return -1;
	return atoi(string);
}


int main(int argc, char **argv)
{
	const char *help =
		"usage: powermate-control <DEVICE> < [OPTION] <VALUE> ...>\n"
		"\n"
		"  -b --brightness [0-255]	set static brightness\n"
		"  -m --mode [d|n|m]		set pulsation mode (divide/normal/multiply)\n"
		"  -s --speed [0-510]		set pulsation speed (best results near 255)\n"
		"  -n --pulse-awake [on/off]	activate/deactivate pulsation when host is running\n"
		"  -f --pulse-asleep [on/off]	activate/deactivate pulsation when host is sleeping\n"
		"  -v --version			display program version and copyright\n"
		"  -h --help			display this information";

	const char *version =
		"powermate-control v" VERSION " - Griffin PowerMate control utility\n"
		"Copyright(C) 2004, 2005 Manuel Sainz de Baranda\n"
		"Distributed under the terms of the GNU General Public License version 2\n"
		"Website: http://www.nongnu.org/libpowermate/";

	const char *commands[] = {
		"-b", "--brightness",
		"-m", "--mode",
		"-s", "--speed",
		"-n", "--pulse-awake",
		"-f", "--pulse-asleep"
	};

	char c;
	int a, v, retval = 0;
	unsigned int i, index, cmd;
	PowerMate *pm;
	PowerMateLED led = {255, 255, 1, 0, 0};

	if (argc == 1 || (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))) {
		puts(help);
		return 0;
	}

	if (argc == 2 && (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))) {
		puts(version);
		return 0;
	}

	if ((pm = powermate_new(argv[1], NULL)) == NULL) {
		switch (errno) {
			case ENODEV:
				printf("error: invalid PowerMate device \"%s\"\n", argv[1]);
				return ENODEV;
			default:
				printf(	"error: can not initialize PowerMate device on \"%s\", errno = %d (%s)\n",
					argv[1], errno, strerror(errno)
				);
				return -errno;
		}
	}

	if (argc == 2) printf("%s\n", pm->model_id);

	else {	if (pm->output == -1) {
			printf("error: read-only PowerMate device \"%s\"\n", argv[1]);
			retval = EACCES;
			goto end;
		}

		for (a = 2; a < argc; a += 2) {
			cmd = index = COMMANDS;

			while (index--) if (!strcmp(argv[a], commands[index])) {
				cmd = index >> 1;
				break;
			}

			if (cmd == COMMANDS) {
				printf("error: invalid option \"%s\"\n", argv[a]);
				return EINVAL;
			}

			if (a + 1 != argc) switch (cmd) {
				case BRIGHTNESS:
					if ((v = get_integer(argv[a + 1])) == -1 || v > 255) break;
					led.static_brightness = (unsigned char) v;
					continue;

				case MODE:
					if (	strlen(argv[a + 1]) > 1 || 
						((c = argv[a + 1][0]) != 'd' && c != 'n' && c != 'm')
					) break;
					led.pulse_table = (c & 3) ? ~c & 3 : 0; /* :-) */
					continue;

				case SPEED:
					if ((v = get_integer(argv[a + 1])) == -1 || v > 1000) break;
					led.pulse_speed = (unsigned short) v;
					continue;

				case PULSE_AWAKE:
					if (!strcmp(argv[a + 1], "on")) led.pulse_awake = POWERMATE_PULSE_STATE_ON;
					else if (!strcmp(argv[a + 1], "off")) led.pulse_awake = POWERMATE_PULSE_STATE_OFF;
					else break;
					continue;

				case PULSE_ASLEEP:
					if (!strcmp(argv[a + 1], "on")) led.pulse_asleep = POWERMATE_PULSE_STATE_ON;
					else if (!strcmp(argv[a + 1], "off")) led.pulse_asleep = POWERMATE_PULSE_STATE_OFF;
					else break;
					continue;
			}

			printf("error: bad sintax in \"%s\"\n", argv[a]);
			return EINVAL;
		}

		if (powermate_set_led(pm, &led)) {
			printf("error: can not configure PowerMate LED, errno = %d (%s)\n", errno, strerror(errno));
			retval = -errno;
		}
	}

	end:
	powermate_destroy(pm);
	return retval;
}


/* powermate-control.c EOF */
