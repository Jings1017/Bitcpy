#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

void v2_bitcpy(void *_dest,      /* Address of the buffer to write to */
            size_t _write,    /* Bit offset to start writing to */
            const void *_src, /* Address of the buffer to read from */
            size_t _read,     /* Bit offset to start reading from */
            size_t count)
{
    const uint8_t MASK[8] = { 1, 3, 7, 15, 31, 63, 127, 255 };
    uint8_t *src = (uint8_t *)_src + (_read >> 3);
    uint8_t src_off = _read & 7;
    uint8_t *dest = (uint8_t *)_dest + (_write >> 3);
    uint8_t dest_off = _write & 7;
    size_t offset = src_off - dest_off;
    
    count -= (8 - dest_off);
    
    int counter = count >> 3;
    int remaining = (count & 7);

    /* step 1 */

    uint8_t data  = (*src++ << offset) ;
    data |= (*src >> (8 - offset));
    data &= MASK[7 - dest_off];
    *dest &= ~MASK[7 - dest_off];  // new 
    *dest++ |= data;
    
    /* step 2 */

    while (counter-- > 0){
        *dest++ = (*src++ << offset) | ((*src >> (8 - offset)) & MASK[offset - 1]);
    }

    /* step 3 */

    if (remaining >= 0) {
        *dest &= MASK[7-remaining]; // new 
        *dest |= (((*src++ << offset) | (*src >> (8 - offset))) & (~MASK[7 - remaining]));
    }
}

void v1_bitcpy(void *_dest,      /* Address of the buffer to write to */
            size_t _write,    /* Bit offset to start writing to */
            const void *_src, /* Address of the buffer to read from */
            size_t _read,     /* Bit offset to start reading from */
            size_t count)
{
    size_t bitsize;
    uint8_t data, original, mask;
    size_t read_lhs = _read & 7;
    size_t read_rhs = 8 - read_lhs;
    const uint8_t *source = (const uint8_t *) _src + (_read >>3);
    size_t write_lhs = _write & 7;
    size_t write_rhs = 8 - write_lhs;
    uint8_t *dest = (uint8_t *) _dest + (_write >>3);
    static const uint8_t MASK[9] = { 255, 127, 63, 31, 15, 7, 3, 1, 0 };

    while (count > 0) {
        // read data
        bitsize = 8^((count^8)&(-(((count-8)&(1<<31))>>63)));
        data = (*source++ << read_lhs) | (*source >> read_rhs);
        data &= ~MASK[bitsize];

        // write data
        mask = ~MASK[write_lhs];
        *dest++ = (*dest & mask) | (data >> write_lhs);
        *dest &= MASK[bitsize - write_rhs];
        *dest |= (data << write_rhs);

        count -= bitsize;
    }
}

void bitcpy(void *_dest,      /* Address of the buffer to write to */
            size_t _write,    /* Bit offset to start writing to */
            const void *_src, /* Address of the buffer to read from */
            size_t _read,     /* Bit offset to start reading from */
            size_t count)
{
    size_t read_lhs = _read & 7;
    size_t read_rhs = 8 - read_lhs;
    const uint8_t *source = (const uint8_t *) _src + (_read / 8);
    size_t write_lhs = _write & 7;
    size_t write_rhs = 8 - write_lhs;
    uint8_t *dest = (uint8_t *) _dest + (_write / 8);

    static const uint8_t read_mask[] = {
        0x00, /*    == 0    00000000b   */
        0x80, /*    == 1    10000000b   */
        0xC0, /*    == 2    11000000b   */
        0xE0, /*    == 3    11100000b   */
        0xF0, /*    == 4    11110000b   */
        0xF8, /*    == 5    11111000b   */
        0xFC, /*    == 6    11111100b   */
        0xFE, /*    == 7    11111110b   */
        0xFF  /*    == 8    11111111b   */
    };

    static const uint8_t write_mask[] = {
        0xFF, /*    == 0    11111111b   */
        0x7F, /*    == 1    01111111b   */
        0x3F, /*    == 2    00111111b   */
        0x1F, /*    == 3    00011111b   */
        0x0F, /*    == 4    00001111b   */
        0x07, /*    == 5    00000111b   */
        0x03, /*    == 6    00000011b   */
        0x01, /*    == 7    00000001b   */
        0x00  /*    == 8    00000000b   */
    };

    while (count > 0) {
        uint8_t data = *source++;
        size_t bitsize = (count > 8) ? 8 : count;
        if (read_lhs > 0) {
            data <<= read_lhs;
            if (bitsize > read_rhs)
                data |= (*source >> read_rhs);
        }

        if (bitsize < 8)
            data &= read_mask[bitsize];

        uint8_t original = *dest;
        uint8_t mask = read_mask[write_lhs];
        if (bitsize > write_rhs) {
            /* Cross multiple bytes */
            *dest++ = (original & mask) | (data >> write_lhs);
            original = *dest & write_mask[bitsize - write_rhs];
            *dest = original | (data << write_rhs);
        } else {
            // Since write_lhs + bitsize is never >= 8, no out-of-bound access.
            mask |= write_mask[bitsize - write_lhs];
            *dest++ = (original & mask) | (data >> write_lhs);
        }

        count -= bitsize;
    }
}

static uint8_t output[1000], input[1000];

static inline void dump_8bits(uint8_t _data)
{   
    for (int i = 0; i < 8; ++i)
        printf("%d", (_data & (0x80 >> i)) ? 1 : 0);
}

static inline void dump_binary(uint8_t *_buffer, size_t _length)
{   
    for (int i = 0; i < _length; ++i)
        dump_8bits(*_buffer++);
}


int main()
{
    struct timespec t1,t2;
    FILE *fp = fopen("raw.txt","w");
    for (int size = 8; size < 7900; size++) {
        memset(&output[0], 0x00, sizeof(output));
        clock_gettime(CLOCK_REALTIME, &t1);
        bitcpy(&output[0], 2, &input[0], 4, size);
        clock_gettime(CLOCK_REALTIME, &t2);
        printf( "%lu\n", t2.tv_nsec - t1.tv_nsec);
        fprintf(fp, "%d\t", size);
        fprintf(fp, "%lu\t", t2.tv_nsec - t1.tv_nsec);
        
        
        memset(&output[0], 0x00, sizeof(output));
        clock_gettime(CLOCK_REALTIME, &t1);
        v1_bitcpy(&output[0], 2, &input[0], 4, size);
        clock_gettime(CLOCK_REALTIME, &t2);
        printf( "%lu\n", t2.tv_nsec - t1.tv_nsec);
        fprintf(fp, "%lu\n", t2.tv_nsec - t1.tv_nsec);
    }
    fclose(fp);
    return 0;
}

/*
int main(int _argc, char **_argv)
{
    memset(&input[0], 0xFF, sizeof(input));

    for (int i = 1; i <= 32; ++i) {
        for (int j = 0; j < 16; ++j) {
            for (int k = 0; k < 16; ++k) {
                memset(&output[0], 0x00, sizeof(output));
                printf("%2d:%2d:%2d ", i, k, j);
                bitcpy(&output[0], k, &input[0], j, i);
                dump_binary(&output[0], 8);
                printf("\n");
            }
        }
    }
    return 0;
}*/