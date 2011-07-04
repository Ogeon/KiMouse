/*
 * main.c
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
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <libfreenect.h>
#include <math.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define FREENECT_FRAME_W        640
#define FREENECT_FRAME_H        480
#define FREENECT_FRAME_PIX      640*480

int g_argc;
char **g_argv;

int window;

freenect_context *f_ctx;
freenect_device *f_dev;

int freenect_angle = 0;

// back: owned by libfreenect (implicit for depth)
// mid: owned by callbacks, "latest frame ready"
// front: owned by GL, "currently being drawn"
uint8_t *depth_mid, *depth_front;
IplImage * image, *detection;
uint16_t *depth;

int got_depth, processed_depth;

GLuint gl_depth_tex;

pthread_t freenect_thread;
pthread_mutex_t gl_backbuf_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t gl_frame_cond = PTHREAD_COND_INITIALIZER;

volatile int die = 0;

uint16_t t_gamma[2048];

unsigned int num_objects;



void *freenect_threadfunc(void *arg);

void depth_cb(freenect_device *dev, void *v_depth, uint32_t timestamp);
void laplace(uint16_t *source, char *dest, uint16_t treshold);
void depth_filter(char *img, uint16_t *depth, int limit);
void thinning(char *img);
void fat_lines(char *image);
void flood_fill(char *img, char value, long int position);

void *gl_threadfunc(void *arg);
void DrawGLScene();
void ReSizeGLScene(int Width, int Height);
void InitGL(int Width, int Height);

void keyPressed(unsigned char key, int x, int y);
void closeWindow();

int main(int argc, char **argv)
{
    int res = 0;

    g_argc = argc;
    g_argv = argv;

    int i;
    for (i=0; i<2048; i++) {
        float v = i/2048.0;
        v = powf(v, 3)* 6;
        t_gamma[i] = v*6*256;
    }

    depth_mid = (uint8_t*)malloc(640*480*3);
    depth_front = (uint8_t*)malloc(640*480*3);
    image = cvCreateImageHeader(cvSize(640, 480), IPL_DEPTH_8U, 3);
    detection = cvCreateImageHeader(cvSize(640, 480), IPL_DEPTH_8U, 3);
    detection->imageData = (char*)malloc(640*480*3);

	if (freenect_init(&f_ctx, NULL) < 0) {
		printf("freenect_init() failed\n");
		return 1;
	}

	freenect_set_log_level(f_ctx, FREENECT_LOG_DEBUG);

	int nr_devices = freenect_num_devices (f_ctx);
	printf ("Number of devices found: %d\n",  nr_devices);

	int user_device_number = 0;

	if (nr_devices < 1)
		return 1;

    printf("Connecting to device number %d... ", user_device_number+1);
	if (freenect_open_device(f_ctx, &f_dev, user_device_number) < 0) {
		printf("Could not open device\n");
		return 1;
	}
	printf("[DONE]\n");

    printf("Creating thread... ");
    res = pthread_create(&freenect_thread, NULL, freenect_threadfunc, NULL);
    if (res) {
        printf("pthread_create failed\n");
        return 1;
    }
	printf("[DONE]\n");

	gl_threadfunc(NULL);

	return (0);
}

void *freenect_threadfunc(void *arg){
    int accelCount = 0, trackingCount = 0;

    freenect_set_tilt_degs(f_dev,freenect_angle);
    freenect_set_led(f_dev,LED_RED);
    freenect_set_depth_callback(f_dev, depth_cb);
    freenect_set_depth_mode(f_dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));

    freenect_start_depth(f_dev);

    while (!die && freenect_process_events(f_ctx) >= 0) {
        if (accelCount++ >= 2000)
        {
            accelCount = 0;
            freenect_raw_tilt_state* state;
            freenect_update_tilt_state(f_dev);
            state = freenect_get_tilt_state(f_dev);
            double dx,dy,dz;
            freenect_get_mks_accel(state, &dx, &dy, &dz);
            printf("\r raw acceleration: %4d %4d %4d mks acceleration: %4f %4f %4f", state->accelerometer_x, state->accelerometer_y, state->accelerometer_z, dx, dy, dz);
            fflush(stdout);
        }

        if(trackingCount++ > 20){
            trackingCount = 0;
            if(depth != NULL && got_depth){
                pthread_mutex_lock(&gl_backbuf_mutex);
                IplImage *grayImg = cvCreateImage( cvSize(image->width, image->height), IPL_DEPTH_8U, 1);
                grayImg->imageData = (char*)malloc(640*480);

                cvCvtColor(image, grayImg, CV_BGR2GRAY);

                depth_filter(grayImg->imageData, depth, 750);
                //cvSmooth(grayImg, grayImg, CV_BLUR, 5, 5, 0, 0);
                //cvThreshold(grayImg, grayImg, 128, 255, CV_THRESH_BINARY);
                thinning(grayImg->imageData);

                cvCvtColor(grayImg, detection, CV_GRAY2BGR);


                free(grayImg->imageData);
                cvReleaseImageData(grayImg);

                got_depth = 0;
                processed_depth++;
                pthread_cond_signal(&gl_frame_cond);
                pthread_mutex_unlock(&gl_backbuf_mutex);
            }
        }
    }


	printf("\nshutting down streams...\n");

    freenect_set_led(f_dev,LED_BLINK_GREEN);
    freenect_set_tilt_degs(f_dev, 0);

    freenect_stop_depth(f_dev);

    freenect_close_device(f_dev);
    freenect_shutdown(f_ctx);

    printf("-- done!\n");

    return NULL;
}

void depth_cb(freenect_device *dev, void *v_depth, uint32_t timestamp){
    int i;
    depth = (uint16_t*)v_depth;
    pthread_mutex_lock(&gl_backbuf_mutex);

    uint16_t d_min = 2047;
    uint16_t d_max = 0;

    //printf("%d\n", num_objects);
    /*for (i=0; i<FREENECT_FRAME_PIX; i++) {
        if(depth[i] < 2047){
            d_min = fmin(d_min, depth[i]);
            d_max = fmax(d_max, depth[i]);
        }
    }

    for (i=0; i<FREENECT_FRAME_PIX; i++) {
        /*int pval = t_gamma[depth[i]];
        int lb = pval & 0xff;*/
        /*int lum = 0;
        if(depth[i] < 2047)
            lum = (int)(255-(depth[i] - d_min)/((float)d_max - d_min) * 255);

        depth_mid[3*i+0] = lum;
        depth_mid[3*i+1] = lum;
        depth_mid[3*i+2] = lum;
    }*/



    image->imageData = depth_mid;
    image->imageDataOrigin = image->imageData;
    //cvCopyImage(detection, image);

    IplImage *grayImg = cvCreateImage( cvSize(image->width, image->height), IPL_DEPTH_8U, 1);
    grayImg->imageData = (char*)malloc(640*480);

    laplace(depth, grayImg->imageData, 10);

    cvCvtColor(grayImg, image, CV_GRAY2BGR);

    free(grayImg->imageData);
    cvReleaseImageData(grayImg);

    got_depth++;
    pthread_cond_signal(&gl_frame_cond);
    pthread_mutex_unlock(&gl_backbuf_mutex);
}

void laplace(uint16_t *source, char *dest, uint16_t treshold){
    int x, y;
    int w = FREENECT_FRAME_W;
    int h = FREENECT_FRAME_H;

    for(y = 0; y < h; y++){
        for(x = 0; x < w; x++){
            int i = y * w + x, sum = 4 * source[i];

            if(x > 0){
                sum -= source[i - 1];
            }else{
                sum -= source[i];
            }

            if(y > 0){
                sum -= source[(y-1) * w + x];
            }else{
                sum -= source[i];
            }

            if(x < w-1){
                sum -= source[i + 1];
            }else{
                sum -= source[i];
            }

            if(y < h-1){
                sum -= source[(y+1) * w + x];
            }else{
                sum -= source[i];
            }
            if(source[i] != 2047){
                int lum = abs(sum) < treshold ? 255: 0;
                dest[i] = lum;
            }else{
                dest[i] = 0;
            }
        }
    }
}

void depth_filter(char *img, uint16_t *depth, int limit){
    int i;

    for(i = 0; i < FREENECT_FRAME_PIX; i++){
        if(depth[i] < limit && img[i] == -1){
            flood_fill(img, (char)(1), i);
        }
    }

    for(i = 0; i < FREENECT_FRAME_PIX; i++){
        if(img[i] == (char)(1)){
            img[i] = 255;
        }else{
            img[i] = 0;
        }
    }
}

void thinning(char *img){
    int deleted, pass = 0;
    int x, y;
    int w = FREENECT_FRAME_W;
    int h = FREENECT_FRAME_H;

    int* limits = calloc(sizeof(int), 2*h);
    int* tmp_limits = calloc(sizeof(int), 2*h);
    for(y = 0; y < h; y++){
        tmp_limits[2*y] = 0;
        tmp_limits[2*y+1] = w;
    }

    do{
        deleted = 0;

        int* tmp = tmp_limits;
        tmp_limits = limits;
        limits = tmp;
        for(y = 0; y < h; y++){
            tmp_limits[2*y] = w;
            tmp_limits[2*y+1] = 0;
        }

        for(y = 0; y < h; y++){
            for(x = limits[2*y]; x < limits[2*y+1]+1; x++){
                if(img[y * w + x] != 0){
                    int a = 0, b = 0, n;
                    int p[8];

                    if(y > 0){
                        p[0] = (img[(y-1) * w + x] != 0)?1:0;

                        if(x > 0){
                            p[7] = (img[(y-1) * w + x-1] != 0)?1:0;
                        }else{
                            p[7] = 0;
                        }

                        if(x < w-1){
                            p[1] = (img[(y-1) * w + x+1] != 0)?1:0;
                        }else{
                            p[1] = 0;
                        }
                    }else{
                        p[7] = 0;
                        p[0] = 0;
                        p[1] = 0;
                    }

                    if(y < h-1){
                        p[4] = (img[(y+1) * w + x] != 0)?1:0;

                        if(x > 0){
                            p[5] = (img[(y+1) * w + x-1] != 0)?1:0;
                        }else{
                            p[5] = 0;
                        }

                        if(x < w-1){
                            p[3] = (img[(y+1) * w + x+1] != 0)?1:0;
                        }else{
                            p[3] = 0;
                        }
                    }else{
                        p[3] = 0;
                        p[4] = 0;
                        p[5] = 0;
                    }

                    if(x > 0){
                        p[6] = (img[y * w + x-1] != 0)?1:0;
                    }else{
                        p[6] = 0;
                    }

                    if(x < w-1){
                        p[2] = (img[y * w + x+1] != 0)?1:0;
                    }else{
                        p[2] = 0;
                    }

                    for(n = 0; n < 8; n++){
                        int n2 = (n+1)%8;
                        b+=p[n];
                        if(p[n] == 1 && p[n2] == 0)
                            a++;
                    }

                    //printf("a=%d, b=%d\n", a, b);
                    if(pass == 0){
                        if(a == 1 && b >= 4 && b <= 7 && p[0]*p[2]*p[4] == 0 && p[2]*p[4]*p[6] == 0){
                            img[y * w + x] = -2;
                            deleted=1;
                        }
                    }else{
                        if(a == 1 && b >= 5 && b <= 7 && p[0]*p[2]*p[6] == 0 && p[0]*p[4]*p[6] == 0){
                            img[y * w + x] = -2;
                            deleted=1;
                        }
                    }
                }
                //printf("%d\n", img[y * w + x]);
            }
        }
        //printf("%d\n", deleted);

        if(deleted == 0)
            break;
        else
            pass = (pass == 0)?1:0;

        for(y = 0; y < h; y++){
            for(x = limits[2*y]; x < limits[2*y+1]+1; x++){
                if(img[y * w + x] == -2){
                    img[y * w + x] = 0;
                }else if(img[y * w + x] != 0){
                    tmp_limits[2*y] = (tmp_limits[2*y] > x)? x: tmp_limits[2*y];
                    tmp_limits[2*y+1] = (tmp_limits[2*y+1] < x)? x: tmp_limits[2*y+1];
                }
            }
        }

    }while(deleted > 0);

    free(limits);
    free(tmp_limits);
}

void fat_lines(char *image){
    int x, y;
    int w = FREENECT_FRAME_W;
    int h = FREENECT_FRAME_H;

    for(y = 0; y < h; y++){
        for(x = 0; x < w; x++){
            int i = y * w + x;

            if(image[i] == 255){
                if(x > 0){
                    image[i - 1] = 255;
                    if(y > 0){
                        image[(y-1) * w + x-1] = 255;
                    }

                    if(y < h-1){
                        image[(y+1) * w + x-1] = 255;
                    }
                }

                if(y > 0){
                    image[(y-1) * w + x] = 255;
                }

                if(x < w-1){
                    image[i + 1] = 0;
                    if(y > 0){
                        image[(y-1) * w + x+1] = 255;
                    }

                    if(y < h-1){
                        image[(y+1) * w + x+1] = 255;
                    }
                }

                if(y < h-1){
                    image[(y+1) * w + x] = 255;
                }
            }
        }
    }
}

void flood_fill(char *img, char value, long int position){
    int w = FREENECT_FRAME_W;
    int h = FREENECT_FRAME_H;
    int x = position % w;
    int y = position/w;

    if(position >= FREENECT_FRAME_PIX || img[position] == value)
        return;

    char old_color = img[position];
    int i = 0;
    //int left = 0;
    //int right = 0;
    int stop = 0;
    //img[position] = value;

    //move left
    while(stop == 0){
        if(x+i>=0){
            if(img[y*w + x+i] == old_color){
                img[y*w + x+i] = value;
            }else{
                stop = 1;
            }
        }else{
            stop = 1;
        }

        i--;
    }

    while(i < 0){
        i++;

        if(y+1 < h){
            if(img[(y+1)*w + x+i] == old_color)
                flood_fill(img, value, (y+1)*w + x+i);
        }

        if(y > 0){
            if(img[(y-1)*w + x+i] == old_color)
                flood_fill(img, value, (y-1)*w + x+i);
        }
    }

    stop = 0;

    while(stop == 0){
        i++;
        if(x+i<w){
            if(img[y*w + x+i] == old_color){
                img[y*w + x+i] = value;
            }else{
                stop = 1;
            }
        }else{
            stop = 1;
        }
    }

    while(i > 1){
        i--;

        if(y+1 < h){
            if(img[(y+1)*w + x+i] == old_color)
                flood_fill(img, value, (y+1)*w + x+i);
        }

        if(y > 0){
            if(img[(y-1)*w + x+i] == old_color)
                flood_fill(img, value, (y-1)*w + x+i);
        }
    }

    /*while(left == 0 && right == 0){
        int x_left = x-i;
        int x_right = x+i;

        if(!left){
            if(x_left < 0 || img[y*w + x_left] != old_color){
                left = 1;
            }else{
                img[y*w + x_left] = value;

                if(y+1 < h){
                    if(img[(y+1)*w + x_left] == old_color)
                        flood_fill(img, value, (y+1)*w + x_left);
                }

                if(y > 0){
                    if(img[(y-1)*w + x_left] == old_color)
                        flood_fill(img, value, (y-1)*w + x_left);
                }
            }
        }

        if(!right){
            if(x_right >= w || img[y*w + x_right] != old_color){
                right = 1;
            }else{
                img[y*w + x_right] = value;

                if(y+1 < h){
                    if(img[(y+1)*w + x_right] == old_color)
                        flood_fill(img, value, (y+1)*w + x_right);
                }

                if(y > 0){
                    if(img[(y-1)*w + x_right] == old_color)
                        flood_fill(img, value, (y-1)*w + x_right);
                }
            }
        }

        i++;
    }*/
}

void *gl_threadfunc(void *arg)
{
printf("GL thread\n");

glutInit(&g_argc, g_argv);

glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
glutInitWindowSize(640, 480);
glutInitWindowPosition(0, 0);

window = glutCreateWindow("Kimouse - Camera View");

glutDisplayFunc(&DrawGLScene);
glutIdleFunc(&DrawGLScene);
glutReshapeFunc(&ReSizeGLScene);
glutKeyboardFunc(&keyPressed);
glutWMCloseFunc(&closeWindow);

InitGL(640, 480);

glutMainLoop();

return NULL;
}

void DrawGLScene(){
    uint8_t *tmp;

    if (processed_depth) {
        tmp = depth_front;
        depth_front = detection->imageData;
        detection->imageData = tmp;
        processed_depth = 0;
    }

    pthread_mutex_unlock(&gl_backbuf_mutex);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, depth_front);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(255.0f, 255.0f, 255.0f, 255.0f);
    glTexCoord2f(1, 0); glVertex3f(0,0,0);
    glTexCoord2f(0, 0); glVertex3f(640,0,0);
    glTexCoord2f(0, 1); glVertex3f(640,480,0);
    glTexCoord2f(1, 1); glVertex3f(0,480,0);
    glEnd();

    glutSwapBuffers();
}

void ReSizeGLScene(int Width, int Height){
    glViewport(0,0,Width,Height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho (0, 640, 480, 0, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
}

void InitGL(int Width, int Height){
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);
    glGenTextures(1, &gl_depth_tex);
    glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ReSizeGLScene(Width, Height);
}

void keyPressed(unsigned char key, int x, int y){
    if (key == 27) {
        die = 1;
        pthread_join(freenect_thread, NULL);
        glutDestroyWindow(window);
        //cvReleaseImage(&image);
        free(depth_mid);
        free(depth_front);
        pthread_exit(NULL);
    }
    if (key == 'w') {
        freenect_angle++;
        if (freenect_angle > 30) {
            freenect_angle = 30;
        }
    }
    if (key == 's') {
        freenect_angle = 0;
    }
    if (key == 'x') {
        freenect_angle--;
        if (freenect_angle < -30) {
            freenect_angle = -30;
        }
    }

    freenect_set_tilt_degs(f_dev,freenect_angle);
}

void closeWindow(){
    keyPressed(27, 0, 0);
}
