#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mmu.h"

#define MAX_ADDRESSES (1 << 10) + 1
#define SEQ_ADDRESSES 1 << 7

int main(int argc, char *argv[])
{
    // check if there is a passed argument, default behaivour sequential access
    int random = 0;
    if (argc > 1)
    {
        if (argv[1][0] == 'r')
            random = 1;
        if (argv[1][0] == 's')
            random = 0;
        else
        {
            printf("Usage: mmu_test <r | s>, use r for random access, use s for sequental access\n");
            return 0;
        }
    }
    MMU mmu = initMemory();
    printf("VIRTUAL MEMORY SIZE: %d\n", VIRTUAL_MEMORY_SIZE);
    printf("PAGE SIZE: %d, NUMBER OF PAGES:%d\n", PAGE_FRAME_SIZE, PAGES_NUM);
    // random access
    if (random)
    {
        srand(time(NULL));
        printf("Generating random addresses...\n");
        VirtualAddress randomAddresses[MAX_ADDRESSES];
        for (int i = 0; i < MAX_ADDRESSES; i++)
        {
            randomAddresses[i].address = rand() % (0xFFFFFF + 1);
            printf("0x%06x,\t", randomAddresses[i].address);
        }
        printf("\n");
        for (int i = 0; i < MAX_ADDRESSES; i++)
        {
            PhysicalAddress a = getPhysicalAddress(&mmu, randomAddresses[i]);
            printf("Virtual: 0x%06x, \tPhysical:0x%06x\n", randomAddresses[i].address, a.address);
            char to_write = 's';
            printf("Writing %c\n", to_write);
            MMU_writeByte(&mmu, randomAddresses[i].address, to_write);
            char *byte = MMU_readByte(&mmu, randomAddresses[i].address);
            printf("Read byte %c\n", *byte);
        }
        freeMemory(&mmu);
        return 0;
    }
    // sequential access
    printf("Sequential access\n");
    VirtualAddress sequentialAddresses[SEQ_ADDRESSES];
    for (int i = 0; i < SEQ_ADDRESSES; i++)
    {
        sequentialAddresses[i].address = i;
        // printf("0x%06x,\t", sequentialAddresses[i].address);
    }
    printf("\n");
    // only the first addess should generate a page fault for the extreme paging, all the others should not
    char loremIpsum[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                        "Nulla facilisi. Pellentesque habitant morbi tristique "
                        "senectus et netus et malesuada fames ac turpis egestas. "
                        "Vestibulum tristique lectus in aliquet volutpat. Sed nec "
                        "convallis nulla. Sed eget eros non justo fermentum "
                        "consequat et sit amet urna. Vivamus tincidunt finibus "
                        "dignissim. Proin aliquet purus nisl, ac tempor ex "
                        "pellentesque vel. Nunc sollicitudin sem augue, id "
                        "congue dolor consectetur vitae. Duis gravida, tortor "
                        "et lobortis aliquam, dolor purus auctor arcu, in "
                        "consectetur mauris mi non odio. Mauris consequat "
                        "lobortis nisi id varius. Sed condimentum dui vitae "
                        "ex semper, nec blandit risus luctus. Sed malesuada "
                        "facilisis ligula, in pharetra leo ornare in. Nam "
                        "tristique sapien eget erat tristique, id fringilla "
                        "neque lacinia.";
    // writing the first whatever characters of lorem ipsum text
    for (int i = 0; i < SEQ_ADDRESSES; i++)
    {
        MMU_writeByte(&mmu, sequentialAddresses[i].address, loremIpsum[i]);
    }
    // reading from memory, this shouldn't generate page faults ad should print the first 128 characters of lorem ipsum
    printf("Reading from memory...\n");
    for (int i = 0; i < SEQ_ADDRESSES; i++)
    {
        char *byte = MMU_readByte(&mmu, sequentialAddresses[i].address);
        printf("%c", *byte);
    }
    printf("\n");
    freeMemory(&mmu);
    return 0;
}
