/*
 * imageOperations.c
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
 
#include "imageOperations.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

void laplace(uint16_t* source, char* dest, uint16_t treshold){
	int x, y;
	int w = FREENECT_FRAME_W;
	int h = FREENECT_FRAME_H;

	for(y = 0; y < h; y++){
		for(x = 0; x < w; x++){
			int i = y * w + x, sum = 4 * source[i];
			if(source[i] != 2047){
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
				int lum = abs(sum) < treshold ? 255: 0;
				dest[i] = lum;
			}else{
				dest[i] = 0;
			}
		}
	}
}


void thinning(char *img){
	int deleted, pass = 0, passes = 0;
	int x, y;
	int w = FREENECT_FRAME_W;
	int h = FREENECT_FRAME_H;

	int* limits = calloc(sizeof(int), 2*h);
	int* tmp_limits = calloc(sizeof(int), 2*h);
	
	int a, b, n;
	int p[8];
	
	char done = 0;
	
	int index;
	
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
				index = y * w + x;
				if(img[index] != 0){
					thinningGetNeighbourInfo(index, img, p, &a, &b);
					
					if(a == 1 && b <= 7 && b >= 2){
					if(pass == 0){
						if(b >= 4 && p[0]*p[2]*p[4] == 0 && p[2]*p[4]*p[6] == 0){
							img[index] = -2;
							deleted=1;
						}else{
							tmp_limits[2*y] = (tmp_limits[2*y] > x)? x: tmp_limits[2*y];
							tmp_limits[2*y+1] = (tmp_limits[2*y+1] < x)? x: tmp_limits[2*y+1];
						}
					}else{
						if(b <= 6 && p[0]*p[2]*p[6] == 0 && p[0]*p[4]*p[6] == 0){
							img[index] = -2;
							deleted=1;
						}else{
							tmp_limits[2*y] = (tmp_limits[2*y] > x)? x: tmp_limits[2*y];
							tmp_limits[2*y+1] = (tmp_limits[2*y+1] < x)? x: tmp_limits[2*y+1];
						}
					}
					}else{
						tmp_limits[2*y] = (tmp_limits[2*y] > x)? x: tmp_limits[2*y];
						tmp_limits[2*y+1] = (tmp_limits[2*y+1] < x)? x: tmp_limits[2*y+1];
					}
				}
			}
		}

		if(deleted == 0)
			if(++done == 2)
				break;
		
		pass = (pass == 0)?1:0;
		
		for(n = 0; n<FREENECT_FRAME_PIX; n++){
			if(img[n] == -2)
				img[n] = 0;
		}
		
	}while(deleted > 0);

	free(limits);
	free(tmp_limits);
}

void thinningGetNeighbourInfo(int i, char*img, int*p, int*ap, int*bp){
	int w = FREENECT_FRAME_W;
	int h = FREENECT_FRAME_H;
	int y = i/w;
	int x = i-y*w;
	int n, n2, a=0, b=0;
	
	for(n = 0; n < 8; n++)
		p[n] = 0;
	
	if(y > 0){
		p[0] = (img[i-w] != 0)?1:0;

		if(x > 0){
			p[7] = (img[i-w-1] != 0)?1:0;
		}

		if(x < w-1){
			p[1] = (img[i-w+1] != 0)?1:0;
		}
	}

	if(y < h-1){
		p[4] = (img[i+w] != 0)?1:0;

		if(x > 0){
			p[5] = (img[i+w-1] != 0)?1:0;
		}

		if(x < w-1){
			p[3] = (img[i+w+1] != 0)?1:0;
		}
	}

	if(x > 0){
		p[6] = (img[i-1] != 0)?1:0;
	}

	if(x < w-1){
		p[2] = (img[i+1] != 0)?1:0;
	}
	
	for(n = 0; n < 8; n++){
		n2 = (n+1)%8;
		b+=p[n];
		if(p[n] == 1 && p[n2] == 0)
			a++;
	}
	
	*ap = a;
	*bp = b;
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

