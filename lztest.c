#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#if !defined(LZ_ENC) && !defined(LZ_DEC)
#error Define LZ_ENC or LZ_DEC
#endif 

#include "soos/lz.h"

void print_percent(const char* fmt, size_t left, size_t right)
{
    char numbuf[8];
    
    uint64_t ratio = (right * 0x640000) / left;
    
    uint64_t both = (ratio & ~0xFFFF) | (ratio & 0xFFFF);
    both *= 100; // both = (both << 6) + (both << 5) + (both << 2)
    both >>= 16;

    uint32_t bcd = 0;
    for (int i = 0; i < 17; ++i)
    {
        bcd <<= 1;
        bcd |= both >> 16;
        both &= ~0x10000U;
        both <<= 1;
        if (i != 16)
        {
            bcd += 3 * ((((bcd | (bcd >> 1)) & (bcd >> 2)) | (bcd >> 3)) & 0x11111U);
        }
    }
    
    if(ratio < 0x640000)
    {
        numbuf[0] = '0' + ((bcd >> 16) & 0xF);
        numbuf[1] = '0' + ((bcd >> 12) & 0xF);
        numbuf[2] = '0' + ((bcd >>  8) & 0xF);
        numbuf[3] = '.';
        numbuf[4] = '0' + ((bcd >>  4) & 0xF);
        numbuf[5] = '0' + ((bcd      ) & 0xF);
        numbuf[6] = '%';
        numbuf[7] = '\0';
    }
    else
    {
        numbuf[0] = '9';
        numbuf[1] = '9';
        numbuf[2] = '9';
        numbuf[3] = '.';
        numbuf[4] = '9';
        numbuf[5] = '%';
        numbuf[6] = '+';
        numbuf[7] = '\0';
    }
    
    printf(fmt, left, right, numbuf);
}

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        printf("Usage: %s <in> <out>\n", argv[0]);
        return 1;
    }
    
    FILE* f = fopen(argv[1], "rb");
    
    if(!f)
    {
        printf("Failed to open input file (%i): %s\n", errno, strerror(errno));
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    
    uint8_t* inbuf = 0;
    uint8_t* outbuf = 0;
    size_t insize = 0;
    size_t outsize = 0;
    size_t outret = 0;
    
    insize = ftell(f);
    fseek(f, 0, SEEK_SET);
    inbuf = malloc(insize);
    fread(inbuf, insize, 1, f);
    fclose(f);
    f = fopen(argv[2], "wb");
    if(!f)
    {
        printf("Failed to open output file (%i): %s\n", errno, strerror(errno));
        return 1;
    }
    
    #ifdef LZ_ENC
    puts("Calculating output size");
    outsize = lzss_enc(inbuf, 0, insize, 0);
    if(!~outsize)
    {
        puts("Can't compress this file");
        fclose(f);
        return 1;
    }
    
    outbuf = malloc(outsize);
    puts("Compressing");
    outret = lzss_enc(inbuf, outbuf, insize, outsize);
    if(!~outret)
    {
        puts("Compress failure");
        fclose(f);
        return 1;
    }
    print_percent("Size reduce %X --> %X %s\n", insize, outsize);
    #endif
    #ifdef LZ_DEC
    outsize = lzss_dec(inbuf, 0, insize);
    if(!outsize)
    {
        puts("Can't decompress this file");
        fclose(f);
        return 1;
    }
    
    outbuf = malloc(outsize);
    outret = lzss_dec(inbuf, outbuf, insize);
    if(outret != outsize)
    {
        puts("Decompress failure");
        fclose(f);
        return 1;
    }
    print_percent("Size expand %X --> %X %s\n", insize, outsize);
    #endif
    
    fwrite(outbuf, outsize, 1, f);
    fflush(f);
    fclose(f);
    
    return 0;
}
