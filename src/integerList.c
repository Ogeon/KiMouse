/*
 * integerList.c
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
 
#include "integerList.h"

#include <stdlib.h>

IntegerList* IntegerListCreate(int sorted, int uniqueValue){
	IntegerList* list = (IntegerList*)malloc(sizeof(IntegerList));
	
	list->sorted = sorted;
	list->uniqueValue = uniqueValue;
	
	list->start = NULL;
	list->end = NULL;
	
	list->size = 0;
	
	return list;
}

void IntegerListInsert(IntegerList* list, int position, int value){
	if(position >= list->size){
		IntegerListInsertLast(list, value);
	}else if(position < 1){
		IntegerListInsertFirst(list, value);
	}else{
		IntegerListElement* prev = list->start;
		IntegerListElement* next = prev->next;
		int i = 0;
		while(++i < position){
			prev = next;
			next = next->next;
		}
		IntegerListElement* new = (IntegerListElement*)malloc(sizeof(IntegerListElement));
		new->value = value;
		new->next = next;
		prev->next = new;
		list->size ++;
	}
}

void IntegerListInsertFirst(IntegerList* list, int value){
	IntegerListElement* new = (IntegerListElement*)malloc(sizeof(IntegerListElement));
	new->value = value;
	new->next = list->start;
	if(list->end == NULL)
		list->end = new;
	list->start = new;
	list->size ++;
}

void IntegerListInsertLast(IntegerList* list, int value){
	IntegerListElement* new = (IntegerListElement*)malloc(sizeof(IntegerListElement));
	new->value = value;
	new->next = NULL;
	if(list->end == NULL){
		list->start = new;
	}else{
		list->end->next = new;
	}
	list->end = new;
	list->size ++;
}

int IntegerListContains(IntegerList* list, int value){
	return IntegerListIndexOf(list, value) != -1;
}

int IntegerListIndexOf(IntegerList* list, int value){
	if(list->size > 0){
		int index = 0;
		IntegerListElement* element = list->start;
		do{
			if(element->value == value)
				return index;
			
			element = element->next;
			index ++;
		}while(element != NULL);
	}
	return -1;
}

int IntegerListGet(IntegerList* list, int position){
	if(position >= list->size || position < 0){
		return 0;
	}else if(position == 0){
		return IntegerListGetFirst(list);
	}else if(position == list->size - 1){
		return IntegerListGetLast(list);
	}else{
		IntegerListElement* element = list->start->next;
		int i = 0;
		while(++i < position){
			element = element->next;
		}
		return element->value;
	}
}

int IntegerListGetFirst(IntegerList* list){
	if(list->size > 0)
		return list->start->value;
		
	return 0;
}

int IntegerListGetLast(IntegerList* list){
	if(list->size > 0)
		return list->end->value;
		
	return 0;
}

int IntegerListRemove(IntegerList* list, int position){
	if(position >= list->size || position < 0){
		return 0;
	}else if(position == 0){
		return IntegerListRemoveFirst(list);
	}else if(position == list->size - 1){
		return IntegerListRemoveLast(list);
	}else{
		IntegerListElement* prev = list->start;
		IntegerListElement* element = list->start->next;
		int i = 0;
		while(++i < position){
			prev = element;
			element = element->next;
		}
		int value = element->value;
		prev->next = element->next;
		free(element);
		list->size --;
		return value;
	}
}

int IntegerListRemoveFirst(IntegerList* list){
	if(list->size > 0){
		IntegerListElement* element = list->start;
		int value = element->value;
		list->start = element->next;
		free(element);
		list->size --;
		return value;
	}
		
	return 0;
}

int IntegerListRemoveLast(IntegerList* list){
	if(list->size > 0){
		IntegerListElement* prev = NULL;
		IntegerListElement* element = list->start;
		while(element->next != NULL){
			prev = element;
			element = element->next;
		}
		int value = element->value;
		prev->next = NULL;
		list->end = prev;
		free(element);
		list->size --;
		return value;
	}
		
	return 0;
}

void IntegerListDestroy(IntegerList* list){
	if(list->size > 0){
		IntegerListElement* element = list->start;
		IntegerListElement* next;
		do{
			next = element->next;
			free(element);
			element = next;
		}while(element != NULL);
	}
	free(list);
}

