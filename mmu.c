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
    page_faults_num = 0;
    MMU mmu;
    mmu.page_table = (PageTable *)malloc(sizeof(PageTable));
    mmu.swap = (SwapSpace *)malloc(sizeof(SwapSpace));
    mmu.ram = (RAM *)malloc(sizeof(RAM));
    printf("init page table\n");
    // Initialize page table
    mmu.page_table->n_pages = PAGES_NUM;
    for (int i = 0; i < PAGES_NUM; i++)
    {
        mmu.page_table->pages[i].frame_number = 0;
        // extreme demand paging, all frames swapped out of memory initially
        mmu.page_table->pages[i].flags = 0;
    }
    printf("init swap\n");
    // Initialize swap space
    mmu.swap->n_frames = PAGES_NUM;
    for (int i = 0; i < PAGES_NUM; i++)
    {
        mmu.swap->frames[i].base = (i * PAGE_FRAME_SIZE >> (VIRTUAL_ADDRESS_NBITS - FRAME_PAGE_NBITS)) & 0xFF;
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
        mmu.ram->frames[i].base = (i * PAGE_FRAME_SIZE >> (VIRTUAL_ADDRESS_NBITS - FRAME_PAGE_NBITS)) & 0xFF;
        // printf("DEBUG: assigned frame number %x, should be: %x\n", mmu.ram->frames[i].base, i * PAGE_FRAME_SIZE);
        mmu.ram->frames[i].flags = Valid; // in ram valid stands for free
        mmu.ram->frames[i].page_number = -1;
        memset(mmu.ram->frames[i].data, 0, PAGE_FRAME_SIZE);
        pushFrame(mmu.ram->free_frames, &(mmu.ram->frames[i]));
    }

    return mmu;
}

void freeMemory(MMU *mmu)
{
    printf("Page Faults generated: %d\n", page_faults_num);
    free(mmu->page_table);
    free(mmu->swap);
    free(mmu->ram);
}

void MMU_exception(MMU *mmu, VirtualAddress virtual)
{
    int page_number = (virtual.address >> (VIRTUAL_ADDRESS_NBITS - FRAME_PAGE_NBITS)) & 0xFF;
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
            mmu->swap->ram_frames[page_number] = NULL;
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
    // printf("DEBUG: victim frame number: %x\n", victim->base);
    mmu->page_table->pages[page_number].frame_number = victim->base;
    // printf("DEBUG: new page flags: %d\n", mmu->page_table->pages[page_number].flags);
    victim->flags &= ~Valid;
    victim->page_number = page_number;
}

// function to translate virtual address, if it is invalid (swapped out) run the routine to swap in

PhysicalAddress getPhysicalAddress(MMU *mmu, VirtualAddress virtual)
{
    PhysicalAddress *physical = (PhysicalAddress *)malloc(sizeof(VirtualAddress));
    unsigned int page_number = (virtual.address >> (VIRTUAL_ADDRESS_NBITS - FRAME_PAGE_NBITS)) & 0xFF;
    unsigned int offset = virtual.address & 0xFFFF;
    // printf("DEBUG: PAGE_NUM=%X, OFFSET:%X\n", page_number, offset);
    //  check if frame is swapped out
    //  printf("DEBUG: old page flags: %d\n", mmu->page_table->pages[page_number].flags);
    //  printf("DEBUG: page valid ? %d\n", mmu->page_table->pages[page_number].flags & Valid);
    if ((mmu->page_table->pages[page_number].flags & Valid) != 1)
    {
        printf("Frame swapped out, swapping in... ----------------------------------------------------------------------------\n");
        // function generates page fault and manages swap out of frame
        page_faults_num++;
        MMU_exception(mmu, virtual);
    }
    int frame_number = mmu->page_table->pages[page_number].frame_number;
    // printf("DEBUG: FRAME_NUM=%X\n", frame_number);
    //  adding reference to the frame swapped in
    mmu->swap->ram_frames[page_number] = &(mmu->ram->frames[frame_number]);
    physical->address = (frame_number << (VIRTUAL_ADDRESS_NBITS - FRAME_PAGE_NBITS)) | offset;
    return *physical;
}

char *MMU_readByte(MMU *mmu, int pos)
{
    // getting the 24 least significant bits to convert pos in a virtual address
    VirtualAddress virtual;
    virtual.address = pos & 0xFFFFFF;
    // call to function to convert virtual in physical, generates page fault if needed
    PhysicalAddress physical = getPhysicalAddress(mmu, virtual);
    // printf("Reading from physical address 0x%x\n", physical.address);
    // extract frame number and offset from physical address
    int frame_number = (physical.address >> (VIRTUAL_ADDRESS_NBITS - FRAME_PAGE_NBITS)) & 0xFF;
    int offset = physical.address & 0xFFFF;
    // printf("DEBUG: max offset: %d\n", PAGE_FRAME_SIZE);
    // printf("DEBUG: frame number: %d, offset: %d\n", frame_number, offset);
    if (frame_number < 0 || frame_number >= PHY_FRAMES_NUM)
    {
        printf("Invalid frame number.\n");
        return NULL;
    }
    if (offset < 0 || offset >= PAGE_FRAME_SIZE)
    {
        printf("Invalid offset.\n");
        return NULL;
    }
    Frame *frame = &(mmu->ram->frames[frame_number]);
    // return address to copy of byte for safety, can't be modified in main by side effect
    char byte = frame->data[offset];
    // printf("Read byte %c\n", *byte);
    frame->flags |= Read;
    return &byte;
}

void syncSwap(MMU *mmu, Frame *frame)
{
    // copy written data to swap and reset Write bit, no longer desynced
    SwapSpace *swap = mmu->swap;
    for (int i = 0; i < PAGE_FRAME_SIZE; i++)
    {
        swap->frames[frame->page_number].data[i] = frame->data[i];
    }
    frame->flags &= ~Write;
    // printf("Swap space synced!\n");
}

void MMU_writeByte(MMU *mmu, int pos, char c)
{
    VirtualAddress virtual;
    virtual.address = pos & 0xFFFFFF;
    PhysicalAddress physical = getPhysicalAddress(mmu, virtual);
    // printf("Writing %c to physical address 0x%x\n", c, physical.address);
    int frame_number = (physical.address >> (VIRTUAL_ADDRESS_NBITS - FRAME_PAGE_NBITS)) & 0xFF;
    int offset = physical.address & 0xFFFF;
    if (frame_number < 0 || frame_number >= PHY_FRAMES_NUM)
    {
        printf("Invalid frame number.\n");
        return;
    }
    if (offset < 0 || offset >= PAGE_FRAME_SIZE)
    {
        printf("Invalid offset.\n");
        return;
    }
    Frame *frame = &(mmu->ram->frames[frame_number]);
    frame->data[offset] = c;
    frame->flags |= Write;
    syncSwap(mmu, frame);
    // printf("DEBUG: just written %c\n", frame->data[offset]);
    return;
}
