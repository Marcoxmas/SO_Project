#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mmu.h"

#define MAX_ADDRESSES (1 << 9)

int main()
{
    MMU mmu = initMemory();
    printf("Generating random addresses...\n");
    srand(time(NULL));
    VirtualAddress addresses[MAX_ADDRESSES];
    for (int i = 0; i < MAX_ADDRESSES; i++)
    {
        addresses[i].address = rand() % (0xFFFFFF + 1);
        printf("0x%06x,\t", addresses[i].address);
    }
    printf("\n");

    printf("VIRTUAL MEMORY SIZE: %d\n", VIRTUAL_MEMORY_SIZE);
    printf("PAGE SIZE: %d, NUMBER OF PAGES:%d\n", PAGE_FRAME_SIZE, PAGES_NUM);
    for (int i = 0; i < MAX_ADDRESSES; i++)
    {
        PhysicalAddress a = getPhysicalAddress(&mmu, addresses[i]);
        printf("Virtual: 0x%06x, \tPhysical:0x%06x\n", addresses[i].address, a.address);
    }
    freeMemory(&mmu);
    return 0;
}
