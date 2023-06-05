#ifndef ARRAYLIST_H
#define ARRAYLIST_H
#include "mmu.h"
#include "constants.h"
#include <stdio.h>

struct Frame;

typedef struct ArrayList
{
    struct Frame *array[PHY_FRAMES_NUM];
    int size;
} ArrayList;

void initialize(ArrayList *list);
void append(ArrayList *list, struct Frame *element);
struct Frame *get(ArrayList *list, int index);
struct Frame *removeElement(ArrayList *list, int index);
struct Frame *pop(ArrayList *list);
int isEmpty(ArrayList *list);
#endif