/*
 * imageOperations.h
 * Copyright (C) Erik Hedvall 2011 <admin@ogeon.se>
 *
 * kimouse is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * kimouse is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <libfreenect.h>

freenect_context *f_ctx;
freenect_device *f_dev;

uint16_t *depthBack, *depthMid, *depthFront;
int depthReceived;

int main(int argc, char **argv){
	printf("Init freenect...");
	if (freenect_init(&f_ctx, NULL) < 0) {
		printf("\nfreenect_init() failed!\n");
		return 1;
	}
	freenect_set_log_level(f_ctx, FREENECT_LOG_DEBUG);

	printf("\33[2K\rInit freenect done\n");

	printf("Searching for devices...");
	int nr_devices = freenect_num_devices (f_ctx);
	printf ("\33[2K\rNumber of devices found: %d\n",  nr_devices);

	int user_device_number = 0;

	if (nr_devices < 1){
		printf("Please connect a device before running KiMouse!\n");
		return 1;
	}

	printf("Connecting to device number %d... ", user_device_number+1);
	if (freenect_open_device(f_ctx, &f_dev, user_device_number) < 0) {
		printf("\rCould not open device number %d!\n", user_device_number+1);
		return 1;
	}
	printf("\33[2K\rDevice number %d is now connected\n", user_device_number+1);

	

	printf("\nShutting down...\n");
	fflush(stdout);

	freenect_set_led(f_dev,LED_BLINK_GREEN);
	freenect_set_tilt_degs(f_dev, 0);

	freenect_stop_depth(f_dev);

	freenect_close_device(f_dev);
	freenect_shutdown(f_ctx);

	printf("KTHXBYE!\n");	
	return 0;
}
