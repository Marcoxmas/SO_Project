#include "mmu.h"
#include <stdio.h>
#include <stdlib.h>
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

    // Initialize page table
    mmu.page_table->n_pages = PAGES_NUM;
    for (int i = 0; i < PAGES_NUM; i++)
    {
        mmu.page_table->pages[i].frame_number = PAGES_NUM - i - 1;
        mmu.page_table->pages[i].flags = Valid;
    }

    // Initialize swap space
    mmu.swap->n_frames = PAGES_NUM;
    for (int i = 0; i < PAGES_NUM; i++)
    {
        mmu.swap->frames[i].base = i;
        // extreme demand paging, all frames swapped out at start
        mmu.swap->frames[i].flags = 0;                            // if valid bit is set, frame is swapped in
        mmu.swap->ram_frames[i] = (Frame *)malloc(sizeof(Frame)); // if swapped in this points to the relative frame in ram
    }

    // Initialize RAM
    mmu.ram->n_frames = PHY_FRAMES_NUM;
    for (int i = 0; i < PHY_FRAMES_NUM; i++)
    {
        mmu.ram->frames[i].base = i * PAGE_FRAME_SIZE;
        mmu.ram->frames[i].flags = Valid; // in ram valid stands for free
        memset(mmu.ram->frames[i].data, 0, PAGE_FRAME_SIZE);
    }

    return mmu;
}

void freeMemory(MMU *mmu)
{
    free(mmu->page_table);
    free(mmu->swap);
    free(mmu->ram);
}

// function to translate virtual address, if it is invalid (swapped out) run the routine to swap in

PhysicalAddress getPhysicalAddress(MMU *mmu, VirtualAddress virtual)
{
    PhysicalAddress *physical = (PhysicalAddress *)malloc(sizeof(VirtualAddress));
    int page_number = virtual.address >> FRAME_PAGE_NBITS;
    int offset = virtual.address & 0xFFF;
    // printf("DEBUG: PAGE_NUM=%X, OFFSET:%X\n", page_number, offset);
    int frame_number = mmu->page_table->pages[page_number].frame_number;
    if (!(mmu->swap->frames[frame_number].flags & Valid))
    {
        // MMU_exception(mmu, virtual);
        printf("Frame swapped out, swapping in...\n");
        // temp algo first fit
        int i = 0, chosen = 0, none_free = 0;
        Frame victim = mmu->ram->frames[i];
        // look for any free ram frames
        while (!none_free || !chosen)
        {
            // printf("i:%d\n", i);
            if (i >= PHY_FRAMES_NUM - 1)
            {
                none_free = 1;
                break;
            }
            if ((victim.flags & Valid))
            {
                chosen = 1;
                break;
            }
            victim = mmu->ram->frames[++i];
        }
        if (none_free)
        {
            printf("No ram frames free!\n");
            exit(-1); // temp, other actions to take!!
        }
        printf("Chose ram frame n. %d to swap out\n", i);
        mmu->ram->frames[i].flags &= ~Valid;
        mmu->swap->frames[frame_number].flags |= Valid;
        mmu->swap->ram_frames[frame_number] = &(mmu->ram->frames[i]);
        printf("Swapped in!\n");
    }
    // printf("DEBUG: FRAME_NUM=%X\n", frame_number);
    /* char c = mmu->swap->ram_frames[frame_number]->data[offset];
    printf("Read char: %c\n", c); */
    physical->address = (frame_number << FRAME_PAGE_NBITS) | offset;
    return *physical;
}