/*
 * integerList.h
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

#ifndef INTEGER_LIST
#define INTEGER_LIST 
 
typedef struct IntegerListElement{
	int value;
	struct IntegerListElement* next;
}IntegerListElement;

typedef struct IntegerList{
	IntegerListElement* start;
	IntegerListElement* end;
	
	int size;
	
	int sorted;
	int uniqueValue;
}IntegerList;

IntegerList* IntegerListCreate(int sorted, int uniqueValue);

void IntegerListInsert(IntegerList* list, int position, int value);
void IntegerListInsertFirst(IntegerList* list, int value);
void IntegerListInsertLast(IntegerList* list, int value);

int IntegerListContains(IntegerList* list, int value);
int IntegerListIndexOf(IntegerList* list, int value);

int IntegerListGet(IntegerList* list, int position);
int IntegerListGetFirst(IntegerList* list);
int IntegerListGetLast(IntegerList* list);
int IntegerListRemove(IntegerList* list, int position);
int IntegerListRemoveFirst(IntegerList* list);
int IntegerListRemoveLast(IntegerList* list);

void IntegerListDestroy(IntegerList* list);

#endif
