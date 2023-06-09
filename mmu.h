#ifndef MMU_H
#define MMU_H
#include <stdint.h>
#include <stdio.h>
#include "constants.h"
#include "arraylist.h"

uint16_t page_faults_num;
struct FrameList;

// FLAGS for pages
typedef enum
{
    Valid = 0x1,
    Unswappable = 0x2,
    Read = 0x4,
    Write = 0x8
} Flags;

typedef struct VirtualAddress
{
    uint32_t address : 24;
} VirtualAddress;

typedef struct PhysicalAddress
{
    uint32_t address : 24;
} PhysicalAddress;

// descriptor for a physical frame, with a single flag to represent if its free
// if frame is swapped out, its valid flag is unset
typedef struct Frame
{
    uint32_t base : FRAME_PAGE_NBITS;
    uint8_t flags : FRAME_FLAGS_NBITS;
    char data[PAGE_FRAME_SIZE];
    uint16_t page_number : FRAME_PAGE_NBITS;
} Frame;

// entry for page table
typedef struct PageEntry
{
    uint32_t frame_number : FRAME_PAGE_NBITS;
    uint32_t flags : PAGE_FLAGS_NBITS;
} PageEntry;

// ram, physical memory, list of frames with reference to raw memory fd
typedef struct RAM
{
    struct ArrayList *free_frames;
    Frame frames[PHY_FRAMES_NUM]; // lista di frame
    uint32_t n_frames;            // numero di frame
} RAM;

typedef struct SwapSpace
{
    Frame frames[PAGES_NUM];
    Frame *ram_frames[PAGES_NUM]; // for each frame in swap i keep track of wich page in ram it represents, if swapped out this is NULL
    uint32_t n_frames;
} SwapSpace;

// page table, with list of page entries
typedef struct PageTable
{
    PageEntry pages[PAGES_NUM];
    uint32_t n_pages;
} PageTable;

// MMU, with reference to a page table, swap space and to physical memory (RAM)
typedef struct MMU
{
    PageTable *page_table;
    FILE *swap;
    RAM *ram;
} MMU;

MMU initMemory();
void freeMemory(MMU *mmu);
PhysicalAddress getPhysicalAddress(MMU *mmu, VirtualAddress virtual);
void MMU_exception(MMU *mmu, VirtualAddress virtual);
char *MMU_readByte(MMU *mmu, int pos);
void MMU_writeByte(MMU *mmu, int pos, char c);

#endif