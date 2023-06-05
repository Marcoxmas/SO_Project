#include "arraylist.h"
#include "constants.h"

void initialize(ArrayList *list)
{
    for (int i = 0; i < PHY_FRAMES_NUM; i++)
    {
        list->array[i] = NULL;
    }
    list->size = 0;
}

void append(ArrayList *list, struct Frame *element)
{
    if (list->size == PHY_FRAMES_NUM)
    {
        printf("Array list is full. Cannot append element.\n");
        return;
    }

    list->array[list->size] = element;
    list->size++;
}

struct Frame *get(ArrayList *list, int index)
{
    if (index >= 0 && index < list->size)
    {
        return list->array[index];
    }
    printf("Index out of bounds.\n");
    return NULL;
}

struct Frame *removeElement(ArrayList *list, int index)
{
    if (index < 0 || index >= list->size)
    {
        printf("Invalid index. Cannot remove element.\n");
        return NULL;
    }

    Frame *removedFrame = list->array[index];

    // Shift elements to the left starting from the index
    for (int i = index; i < list->size - 1; i++)
    {
        list->array[i] = list->array[i + 1];
    }

    // Clear the last element
    list->array[list->size - 1] = NULL;

    list->size--;

    return removedFrame;
}

struct Frame *pop(ArrayList *list)
{
    return removeElement(list, 0);
}

int isEmpty(ArrayList *list)
{
    return list->size == 0;
}