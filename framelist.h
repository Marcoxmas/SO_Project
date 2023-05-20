#ifndef FRAMELIST_H
#define FRAMELIST_H
#include "mmu.h"

struct Frame;

typedef struct FrameNode
{
    struct Frame *frame;
    struct FrameNode *next;
} FrameNode;

// Linked list structure
typedef struct FrameList
{
    FrameNode *head;
    FrameNode *tail;
} FrameList;

void initializeFrameList(FrameList *list);
int isFrameListEmpty(FrameList *list);
void pushFrame(FrameList *list, struct Frame *frame);
struct Frame *popFrame(FrameList *list);

#endif