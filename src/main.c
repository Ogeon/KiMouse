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
#include <unistd.h>

#include <pthread.h>

#include <libfreenect.h>

#include <GL/glu.h>
#include <GL/glfw.h>


freenect_context *f_ctx;
freenect_device *f_dev;

pthread_t processThread;
pthread_mutex_t gl_backbuf_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t gl_frame_cond = PTHREAD_COND_INITIALIZER;

uint16_t *depthBack, *depthMid, *depthFront;
int depthReceived;
int running;
int cameraAngle;

void* processLoop(void* arg){
	while(running == 1){
		
	}
	return NULL;
}

void initGL(){
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_SMOOTH);
	//glGenTextures(1, &gl_depth_tex);
	//glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void resizeGL(int width, int height){
	if (height == 0) //Prevent divide by 0
		height = 1;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glViewport(0, 0, width, height);

	gluPerspective(45.0f, (float) width / (float) height, 1.0f, 1000.0f);

	glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
	glLoadIdentity();
}

void redraw(){
	
	glfwSwapBuffers();
}

int main(int argc, char **argv){
	printf("Init freenect...");
	if (freenect_init(&f_ctx, NULL) < 0) {
		printf("\n\tfreenect_init() failed!\n");
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
	freenect_set_led(f_dev,LED_GREEN);
	freenect_set_tilt_degs(f_dev, -5);
	sleep(1);
	printf("\nHello!\n\n");
	freenect_set_tilt_degs(f_dev, 0);
	sleep(1);
	freenect_set_led(f_dev,LED_RED);
	//freenect_set_depth_callback(f_dev, depth_cb);
	freenect_set_depth_mode(f_dev,
	    freenect_find_depth_mode(
	        FREENECT_RESOLUTION_MEDIUM,
	        FREENECT_DEPTH_11BIT)
	    );
		
	running = 1;
	
	printf("Creating worker thread... ");
	int res = pthread_create(&processThread, NULL, processLoop, NULL);
	if (res) {
		printf("\n\tpthread_create failed!\n");
		return 1;
	}
	printf("\33[2K\rWorker thread created");
	
	printf("Initiating GLFW...");
	if (!glfwInit()) {
		printf("\n\tglfwInit failed!\n");
		return 1;
	}
	printf("\33[2K\rInit GLFW done");
	
	printf("Opening GLFW window...");
	if (!glfwOpenWindow(
	    640, 480,   // Resolution
	    8, 8, 8,	// Color buffer depth
	    8,		  // Alpha buffer depth
	    24,		  // Depth buffer
	    0,		  // Stencil buffer
	    GLFW_WINDOW)){
		printf("\n\tglfwOpenWindow failed!\n");
		return 1;
	}
	glfwSetWindowTitle("KiMouse");
	glfwSetWindowSizeCallback(resizeGL);
	printf("\33[2K\rGLFW window is open\n");
	
	initGL();
	
	cameraAngle = 0;
	int angleCount = 0;
	while(running){
		redraw();
		if(++angleCount > 10){
			if(glfwGetKey(GLFW_KEY_UP) && cameraAngle < 30){
				cameraAngle ++;
			}
			if(glfwGetKey(GLFW_KEY_SPACE)){
				cameraAngle = 0;
			}
			if(glfwGetKey(GLFW_KEY_DOWN) && cameraAngle > -30){
				cameraAngle --;
			}
			freenect_set_tilt_degs(f_dev, cameraAngle);
			angleCount = 0;
		}
		running = !glfwGetKey( GLFW_KEY_ESC ) &&
		    glfwGetWindowParam( GLFW_OPENED );
	}
	glfwTerminate();
	
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

