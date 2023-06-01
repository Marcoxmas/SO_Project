#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mmu.h"

#define SEQ_ADDRESSES 1 << 11
#define SEQ_ADDRESSES_PF 1 << 20

int main(int argc, char *argv[])
{
    // check if there is a passed argument, default behaivour sequential access
    int random = 0;
    if (argc > 1)
    {
        if (argv[1][0] == 'r')
        {
            printf("selected random\n");
            random = 1;
        }
        else if (argv[1][0] == 's')
            random = 0;
        else
        {
            printf("Usage: mmu_test <r | s>, use r for random access, use s for sequental access\n");
            return 0;
        }
    }
    MMU mmu = initMemory();
    uint64_t MAX_ADDRESSES = 1ULL << 24;
    printf("VIRTUAL MEMORY SIZE: %d\n", VIRTUAL_MEMORY_SIZE);
    printf("PAGE SIZE: %d, NUMBER OF PAGES:%d\n", PAGE_FRAME_SIZE, PAGES_NUM);
    // random access
    if (random)
    {
        srand(time(NULL));
        printf("Generating random addresses...\n");
        VirtualAddress *randomAddresses = malloc(MAX_ADDRESSES * sizeof(VirtualAddress));
        if (randomAddresses == NULL)
        {
            printf("Failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
        // VirtualAddress randomAddresses[MAX_ADDRESSES];
        for (unsigned long long i = 0; i < MAX_ADDRESSES; i++)
        {
            randomAddresses[i].address = rand() % (0xFFFFFF + 1);
            printf("0x%06x,\t", randomAddresses[i].address);
        }
        printf("\n");
        // only tests writing to focus on page faults
        for (unsigned long long i = 0; i < MAX_ADDRESSES; i++)
        {
            // PhysicalAddress a = getPhysicalAddress(&mmu, randomAddresses[i]);
            // printf("Virtual: 0x%06x, \tPhysical:0x%06x\n", randomAddresses[i].address, a.address);
            char to_write = 's';
            // printf("Writing %c\n", to_write);
            // printf("%llu ", i);
            MMU_writeByte(&mmu, randomAddresses[i].address, to_write);
            /* DECOMMENT THIS TO VERIFY THAT AL THE S'S ARE WRITTEN */
            // char *byte = MMU_readByte(&mmu, randomAddresses[i].address);
            // printf("%c", *byte);
            // free(byte);
        }
        printf("\n");
        free(randomAddresses);
        freeMemory(&mmu);
        return 0;
    }
    // sequential access
    printf("Sequential access\n");
    VirtualAddress sequentialAddresses[SEQ_ADDRESSES];
    for (int i = 0; i < SEQ_ADDRESSES; i++)
    {
        sequentialAddresses[i].address = i & 0xFFFFFF;
        // printf("0x%06x,\t", sequentialAddresses[i].address);
    }
    printf("\n");
    // only the first addess should generate a page fault for the extreme paging, all the others should not
    FILE *file = fopen("lorem_ipsum.txt", "r");
    if (file == NULL)
    {
        printf("Failed to open the file.\n");
        return 1;
    }
    // Determine the size of the file
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    // Allocate memory for the char array
    char *loremIpsum = (char *)malloc(fileSize + 1);
    if (loremIpsum == NULL)
    {
        printf("Failed to allocate memory.\n");
        fclose(file);
        return 1;
    }
    // Read the file content into the char array
    fread(loremIpsum, sizeof(char), fileSize, file);
    loremIpsum[fileSize] = '\0';
    fclose(file);
    // writing the first whatever characters of lorem ipsum text
    printf("Writing starting from 0x%06x, to 0x%06x\n", sequentialAddresses[0].address, sequentialAddresses[(SEQ_ADDRESSES)-1].address);
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
        free(byte);
    }
    printf("\n");
    printf("\n\n\nSequential with page faults, starting from last address used\n");
    VirtualAddress sequentialPfAddresses[SEQ_ADDRESSES_PF];
    for (int i = 0; i < SEQ_ADDRESSES_PF; i++)
    {
        sequentialPfAddresses[i].address = (i + (SEQ_ADDRESSES)) & 0xFFFFFF;
        // printf("0x%06x,\t", sequentialPfAddresses[i].address);
    }
    printf("Writing starting from 0x%06x, to 0x%06x\n", sequentialPfAddresses[0].address, sequentialPfAddresses[(SEQ_ADDRESSES_PF)-1].address);
    fflush(stdout);
    for (int i = 0; i < SEQ_ADDRESSES_PF; i++)
    {
        char *byte = MMU_readByte(&mmu, sequentialPfAddresses[i].address);
        free(byte);
    }
    for (int i = 0; i < SEQ_ADDRESSES_PF; i++)
    {
        MMU_writeByte(&mmu, sequentialPfAddresses[i].address, loremIpsum[i]);
    }
    printf("Reading from memory...\n");
    for (int i = 0; i < SEQ_ADDRESSES_PF; i++)
    {
        char *byte = MMU_readByte(&mmu, sequentialPfAddresses[i].address);
        printf("%c", *byte);
        free(byte);
    }
    printf("\n");
    free(loremIpsum);
    freeMemory(&mmu);
    return 0;
}
