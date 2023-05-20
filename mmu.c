#include "mmu.h"
#include "framelist.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

MMU initMemory()
{
    MMU mmu;
    mmu.page_table = (PageTable *)malloc(sizeof(PageTable));
    mmu.swap = (SwapSpace *)malloc(sizeof(SwapSpace));
    mmu.ram = (RAM *)malloc(sizeof(RAM));
    printf("init page table\n");
    // Initialize page table
    mmu.page_table->n_pages = PAGES_NUM;
    for (int i = 0; i < PAGES_NUM; i++)
    {
        mmu.page_table->pages[i].frame_number = PAGES_NUM - i - 1;
        // extreme demand paging, all frames swapped out of memory initially
        mmu.page_table->pages[i].flags = 0;
    }
    printf("init swap\n");
    // Initialize swap space
    mmu.swap->n_frames = PAGES_NUM;
    for (int i = 0; i < PAGES_NUM; i++)
    {
        mmu.swap->frames[i].base = i + PAGE_FRAME_SIZE;
        // extreme demand paging, all frames swapped out at start
        mmu.swap->frames[i].flags = 0;
        mmu.swap->frames[i].page_number = i;
        mmu.swap->ram_frames[i] = (Frame *)malloc(sizeof(Frame)); // if swapped in this points to the relative frame in ram
    }

    // Initialize RAM
    printf("init RAM\n");
    mmu.ram->n_frames = PHY_FRAMES_NUM;
    mmu.ram->free_frames = malloc(sizeof(FrameList));
    initializeFrameList(mmu.ram->free_frames);
    for (int i = 0; i < PHY_FRAMES_NUM; i++)
    {
        mmu.ram->frames[i].base = i * PAGE_FRAME_SIZE;
        mmu.ram->frames[i].flags = Valid; // in ram valid stands for free
        mmu.ram->frames[i].page_number = -1;
        memset(mmu.ram->frames[i].data, 0, PAGE_FRAME_SIZE);
        pushFrame(mmu.ram->free_frames, &(mmu.ram->frames[i]));
    }

    return mmu;
}

void freeMemory(MMU *mmu)
{
    free(mmu->page_table);
    free(mmu->swap);
    free(mmu->ram);
}

void MMU_exception(MMU *mmu, VirtualAddress virtual)
{
    int page_number = virtual.address >> FRAME_PAGE_NBITS;
    int offset = virtual.address & 0xFFF;
    int i = 0, chosen = 0;
    // check if there are any free frames first
    Frame *victim;
    if (!isFrameListEmpty(mmu->ram->free_frames))
    {
        victim = popFrame(mmu->ram->free_frames);
        printf("Free frame found!\n");
        chosen = 1;
    }
    while (!chosen)
    {
        victim = &(mmu->ram->frames[i++]);
        uint8_t flags = victim->flags;
        printf("Frame n.%d flags:%d\n", i - 1, flags);
        if (flags & Unswappable)
        {
            continue;
        }
        // If the read bit is 0 and the write bit is 0, indicating that the page has not been recently accessed and is not modified, the page is selected for replacement.
        if (((flags & Read) == 0) && ((flags & Write) == 0))
        {
            printf("Frame not recently used found!\n");
            chosen = 1;
            continue;
        }
        // If the read bit is 1, indicating that the page has been accessed recently, the read bit is cleared (set to 0), move on to the next page
        if (flags & Read)
        {
            victim->flags &= ~Read;
            continue;
        }
        // If the read bit is 0 but the write bit is 1, indicating that the page has not been recently accessed but has been modified, the algorithm gives the page a higher priority and sets the read bit to 1.
        // the write bit gets reset when the ram frame has been synchronized with the frame on the swap space
        if (flags & Write)
        {
            victim->flags |= Read;
            continue;
        }
        if (i > PHY_FRAMES_NUM - 1)
        {
            i = 0;
            continue;
        }
    }
    // printf("Chose ram frame n. %d to swap out\n", i);
    mmu->page_table->pages[page_number].flags |= Valid;
    victim->flags &= ~Valid;
    victim->page_number = page_number;
    mmu->page_table->pages[victim->page_number].flags &= ~Valid;
}

// function to translate virtual address, if it is invalid (swapped out) run the routine to swap in

PhysicalAddress getPhysicalAddress(MMU *mmu, VirtualAddress virtual)
{
    PhysicalAddress *physical = (PhysicalAddress *)malloc(sizeof(VirtualAddress));
    int page_number = virtual.address >> FRAME_PAGE_NBITS;
    int offset = virtual.address & 0xFFF;
    // printf("DEBUG: PAGE_NUM=%X, OFFSET:%X\n", page_number, offset);
    // check if frame is swapped out
    if (!(mmu->page_table->pages[page_number].flags & Valid))
    {
        printf("Frame swapped out, swapping in...\n");
        // function generates page fault and manages swap out of frame
        MMU_exception(mmu, virtual);
    }
    int frame_number = mmu->page_table->pages[page_number].frame_number;
    // printf("DEBUG: FRAME_NUM=%X\n", frame_number);
    /* char c = mmu->swap->ram_frames[frame_number]->data[offset];
    printf("Read char: %c\n", c); */
    // adding reference to the frame swapped in
    mmu->swap->ram_frames[page_number] = &(mmu->ram->frames[frame_number]);
    physical->address = (frame_number << FRAME_PAGE_NBITS) | offset;
    return *physical;
}