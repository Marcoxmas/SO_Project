#include "framelist.h"
#include <stdlib.h>
void initializeFrameList(FrameList *list)
{
    list->head = NULL;
    list->tail = NULL;
}

// Check if the frame list is empty
int isFrameListEmpty(FrameList *list)
{
    return (list->head == NULL);
}

// Push a frame to the head of the frame list
void pushFrame(FrameList *list, struct Frame *frame)
{
    FrameNode *newNode = (FrameNode *)malloc(sizeof(FrameNode));
    newNode->frame = frame;
    newNode->next = NULL;

    if (isFrameListEmpty(list))
    {
        // The list is empty, so the new node becomes both the head and the tail
        list->head = newNode;
        list->tail = newNode;
    }
    else
    {
        // The new node is added to the head of the list
        newNode->next = list->head;
        list->head = newNode;
    }
}

// Pop the frame at the head of the frame list
struct Frame *popFrame(FrameList *list)
{
    if (isFrameListEmpty(list))
    {
        // The list is empty, no frame to pop
        return NULL;
    }

    FrameNode *popNode = list->head;
    Frame *frame = popNode->frame;

    if (list->head == list->tail)
    {
        // The list has only one node, so it becomes empty after popping
        list->head = NULL;
        list->tail = NULL;
    }
    else
    {
        // Remove the head node and update the head pointer
        list->head = list->head->next;
    }

    free(popNode);
    return frame;
}