#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 256
#define PAGE_TABLE_SIZE 256
#define TLB_SIZE 16

uint8_t page_table[PAGE_TABLE_SIZE][PAGE_SIZE];
int tlb[TLB_SIZE][2];
FILE* backing_store_file;

int tlb_hits = 0;
int page_faults = 0;
int total_addresses = 0;

//initializing the page table
void initialize_page_table() {
    memset(page_table, 0, sizeof(page_table));
}

//initialize page and frame numbers to -1 (invalid)
void initialize_tlb() {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i][0] = -1;
        tlb[i][1] = -1;
    }
}

//open the backing store
void open_backing_store_file() {
    backing_store_file = fopen("BACKING_STORE.bin", "rb");
    if (backing_store_file == NULL) {
        perror("Error opening backing store file");
        exit(EXIT_FAILURE);
    }
}

//close the backing store
void close_backing_store_file() {
    fclose(backing_store_file);
}

//searching for given number and returning if found
void read_page_from_backing_store(uint8_t page_num) {
    if (fseek(backing_store_file, page_num * PAGE_SIZE, SEEK_SET) != 0) {
        perror("Error seeking in backing store file");
        exit(EXIT_FAILURE);
    }
    if (fread(page_table[page_num], sizeof(uint8_t), PAGE_SIZE, backing_store_file) != PAGE_SIZE) {
        perror("Error reading from backing store file");
        exit(EXIT_FAILURE);
    }
}


int search_tlb(int page_num) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i][0] == page_num) {
            return tlb[i][1];
        }
    }
    return -1;
}

void update_tlb(int page_num, int frame_num) {
    for (int i = TLB_SIZE - 1; i > 0; i--) {
        tlb[i][0] = tlb[i - 1][0];
        tlb[i][1] = tlb[i - 1][1];
    }
    tlb[0][0] = page_num;
    tlb[0][1] = frame_num;
}

uint8_t translate_address(uint16_t logical_addr) {
    uint8_t page_num = (logical_addr >> 8) & 0xFF;
    uint8_t offset = logical_addr & 0xFF;

    int frame_num = search_tlb(page_num);
    if (frame_num == -1) {
        if (page_table[page_num][0] == 0) {
            read_page_from_backing_store(page_num);
            page_faults++;
        }
        frame_num = page_num;
        update_tlb(page_num, frame_num);
    } else {
        tlb_hits++;
    }

    uint8_t byte_value = page_table[page_num][offset];
    
    return byte_value;
}

int main() {
    initialize_page_table();
    initialize_tlb();
    open_backing_store_file();

    FILE* addr_file = fopen("addresses.txt", "r");
    if (addr_file == NULL) {
        perror("Error opening addresses file");
        exit(EXIT_FAILURE);
    }

    uint16_t logical_addr;
    while (fscanf(addr_file, "%hu", &logical_addr) != EOF) {
        logical_addr &= 0xFFFF;
        uint8_t byte_value = translate_address(logical_addr);
        printf("Logical Address: %hu, Physical Address: %hu, Value: %d\n", logical_addr, logical_addr, byte_value);
        total_addresses++;
    }

    printf("Page fault rate: %.2f%%\n", (float)page_faults / total_addresses * 100);
    printf("TLB hit rate: %.2f%%\n", (float)tlb_hits / total_addresses * 100);

    fclose(addr_file);
    close_backing_store_file();

    return 0;
}
