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
 
#include <stdint.h>
#include "integerList.h"

void laplace(uint16_t* source, char* dest, uint16_t treshold);

void thinning(char *img);

void thinning2(char *img);
void thinningAddNeighbours(int i, int*p, IntegerList*list);
void thinningGetNeighbourInfo(int i, char*img, int*p, int*ap, int*bp);

void flood_fill(char *img, char value, long int position);

