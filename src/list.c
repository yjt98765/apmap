/*
* Copyright (c) 2019, Delft University of Technology
*
* list.c
*
* Functions related to the list struct
*/
#include "apmapbin.h"

/*
* Create a list
*/
list_t *CreateList(int size)
{
  list_t *list = (list_t*)malloc(sizeof(list_t));
  list->value = (int*)malloc(size * sizeof(int));
  list->maxsize = size;
  list->size = 0;
  return list;
}

/*
* List initialization
*/
void InitList(list_t *list, int size)
{
  list->value = (int*)malloc(size * sizeof(int));
  list->maxsize = size;
  list->size = 0;
}

/*
* Add a new value to the list.
* Return 0 if the value is already in the list; return 1 otherwise.
*/
char ListAddNew(list_t *list, int num)
{
  int i;

  for (i=0; i<list->size; i++) {
    if (num == list->value[i]) {
      return 0;
    }
  }
  list->size++;
  if (list->size > list->maxsize) {
    list->maxsize *= 2;
    list->value = (int*)realloc(list->value, list->maxsize * sizeof(int));
  }
  list->value[i] = num;
  return 1;
}

/*
* Add a new value to the list without checking its appearance.
* Return the new size of the list.
*/
int ListAdd(list_t *list, int num)
{
  int i;
  if (list->size >= list->maxsize) {
    list->maxsize *= 2;
    list->value = (int*)realloc(list->value, list->maxsize * sizeof(int));
  }
  list->value[list->size] = num;
  return list->size++;
}

/*
* Get the last value in the list.
*/
int ListPop(list_t *list)
{
  if (list->size == 0)
  {
    return -1;
  }
  list->size--;
  return list->value[list->size];
}

/*
* Change one value in a list.
* Return 1 if the given value is found; return 0 otherwise.
*/
char ListChange(list_t *list, int origin, int current)
{
  char rst = 0;
  int i;

  for (i=0; i<list->size; i++) {
    if (list->value[i] == origin) {
      list->value[i] = current;
      rst = 1;
    }
  }
  return rst;
}

void ListCopy(list_t *dest, list_t *src)
{
  int i;
  if (dest->maxsize < src->size)
  {
    dest->value = (int*)realloc(dest->value, src->size * sizeof(int));
  }
  for (i=0; i<src->size; i++)
  {
    dest->value[i] = src->value[i];
  }
}

/*
* Swap two values in a list.
* Return 1 if the given value is found; return 0 otherwise.
*/
int SwapByPosValue(int *list, int size, int value, int pos)
{
  int i;

  if (pos >= size) {
    return -1;
  }
  for (i = 0; i < size; i++) {
    if (list[i] == value) {
      list[i] = list[pos];
      list[pos] = value;
      return i;
    }
  }
  return -1;
}

/*
* Delete all the values in a list
*/
void EmptyList(list_t *list)
{
  list->size = 0;
}

/*
* Free the resources in a list
*/
void FreeList(list_t *list)
{
  if (list) {
    free(list->value);
    free(list);
  }
}
