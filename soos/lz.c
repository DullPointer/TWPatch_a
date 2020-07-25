//old shit
//#define LZ_DEBUG
#if 1
//#ifdef _WIN32
#include "C:\\Data\\lolol\\3DS\\TWL\\twltest\\lz.c"
//#else
//#include "/media/Wasteland/Data/lolol/3DS/TWL/twltest/lz.c"
//#endif
//#else
#endif
#if 1

#include <stddef.h>
#include <stdint.h>

#include "lz.h"

//#ifdef LZ_DEBUG
#include <stdio.h>
//#endif

static int nums[64];

static inline __attribute__((optimize("Os"))) size_t lzss_find_buffer2(const uint8_t* in, size_t i, size_t findoffs, size_t* patlen)
{
    size_t insize = *patlen;
    
    size_t o = i + findoffs;
    size_t n = 0;
    
    size_t po = 0;
    size_t pn = 0;
    
    //puts("=");
    while(o > i && i >= n && (o - i) >= 3)
    {
        //printf("lz %X %02X %02X %X %X\n", n, in[i - n], in[o - n], i, o);
        
        if(in[i - n] == in[o - n])
        {
            if(++n != 18)
                continue;
            break;
        }
        
        if(n < 3)
        {
            --o;
            n = 0;
            continue;
        }
        
        //break;
        
        if(n >= pn)
        {
            //printf("set po %X pn %X\n", o, n);
            
            po = o;
            pn = n;
        }
        
        --o;
        n = 0;
    }
    
    if(n < pn)
    {
        o = po;
        n = pn;
    }
    
    if(i >= 0xC0000)
        nums[n]++;
    
    if(n < 3)
    {
        *patlen = 0;
        return 0;
    }
    
    #ifdef LZ_DEBUG
    //printf("findbuf find %X-%X %X-%X %X\n", i, i - n + 1, o, o - n + 1, n);
    #endif
    
    *patlen = n;
    return o - i;
}

size_t __attribute__((optimize("Os"))) lzss_enc2(const void* __restrict in, void* __restrict out, size_t insize, size_t outsize)
{
    const uint8_t* bufin = in;
    uint8_t* bufout = out;
    
    if(!bufout)
        outsize = 0;
    
    size_t i = insize;
    size_t o = outsize - 8;
    
    uint8_t cmd = 0;
    uint8_t cmdi = 8;
    size_t cmdo = --o;
    
    //#ifndef LZ_SILENT
    size_t nextnotif = insize;
    //#endif
    
    size_t cachebuf[256];
    
    memset(cachebuf, 0xFF, sizeof(cachebuf));
    
    memset(nums, 0, sizeof(nums));
    
    while(i)
    {
        uint8_t inbyte = bufin[--i];
        
        size_t findoffs = cachebuf[inbyte] - i;
        if(findoffs > 0x1002)
        {
            if(~cachebuf[inbyte])
            {
                //printf("relocat %02X %03X %X\n", inbyte, findoffs, i);
                
                findoffs = i + 0x1002;
                
                while(bufin[findoffs] != inbyte)
                    findoffs -= 1;
                
                cachebuf[inbyte] = findoffs;
                findoffs -= i;
            }
            else
            {
                cachebuf[inbyte] = i;
                findoffs = 0;
            }
        }
        
        if(findoffs < 3)
        {
            o -= 1;
            if(bufout)
                bufout[o] = inbyte;
            
            cmdi -= 1;
        }
        else
        {
            //printf("findbuf %02X %03X %X\n", inbyte, findoffs, i);
            size_t findlen = insize;
            findoffs = lzss_find_buffer2(bufin, i, findoffs, &findlen);
            if(findoffs)
            {
                if(findlen < 3 || findlen > 18)
                {
                    puts("invalid findlen");
                    return ~0;
                }
                
                //#ifndef LZ_SILENT
                if(!bufout || i > nextnotif)
                {
                    
                }
                else
                {
                    printf("Compressing, 0x%X bytes left...\n", i);
                    if(i > 0x1000)
                        nextnotif = i - 0x1000;
                    else
                        nextnotif = 0;
                }
                //#endif
                
                uint16_t dupdata = findoffs - 3;
                if(dupdata >> 12)
                {
                    printf("lzss_find_buffer broken %X %X %X\n", i, findoffs, findlen);
                    return ~0;
                }
                
                i -= findlen - 1;
                
                dupdata |= (findlen - 3) << 12;
                
                if(bufout)
                {
                    bufout[--o] = (uint8_t)(dupdata >> 8);
                    bufout[--o] = (uint8_t)(dupdata);
                }
                else
                    o -= 2;
                
                cmd |= 1 << (--cmdi);
            }
            else
            {
                o -= 1;
                
                if(bufout)
                    bufout[o] = inbyte;
                
                --cmdi;
            }
        }
        
        if(!bufout || i > o)
        {
            if(!cmdi)
            {
                if(bufout)
                    bufout[cmdo] = cmd;
                
                cmdo = --o;
                
                cmdi = 8;
                cmd = 0;
            }
        }
        else
            break;
    }
    
    for(i = 0; i != sizeof(nums) / sizeof(nums[0]); i++)
        printf("%02i %i\n", i, nums[i]);
    
    if(!bufout)
        return ((-o) + 4) & ~3;
    else if(cmdi != 8)
        bufout[cmdo] = cmd;
    
    if(o)
    {
        i = o;
        while(i--)
            bufout[i] = bufin[i];
    }
    
    bufout += outsize - 8;
    *(uint32_t*)bufout = (8 << 24) | (outsize - o);
    bufout += 4;
    *(uint32_t*)bufout = insize - outsize;
    return o;
}

//copypasted from original lz.c, but slightly edited
size_t lzss_dec2(const void* __restrict in, void* __restrict out, size_t insize)
{
    // size includes header
    if(insize < 8)
        return 0;
    
    // i - input index
    // o - output index
    // e - compressed blob end, memcpy between [0 to e)
    
    // footer
    // -8 0xSSEEEEEE
    //    S = begin of compressed data from the end, must be >= 8
    //    E = (length of compressed data) + S, must be > S
    // -4 decompressed data size öffset, = öriginal size - compressed size
    
    size_t outsize = insize + *(const uint32_t*)(in + insize - 4);
    
    size_t i = insize - (*(const uint32_t*)(in + insize - 8) >> 24);
    size_t o = outsize;
    size_t e = insize - (*(const uint32_t*)(in + insize - 8) & 0xFFFFFF);
    
    if(!out)
        return outsize;
    
    const uint8_t* bufin = in;
    uint8_t* bufout = out;
    
    #ifdef LZ_DEBUG
    size_t conseq0_max = 0;
    size_t conseq1_max = 0;
    size_t conseq0_curr = 0;
    size_t conseq1_curr = 0;
    
    size_t conseq0 = 0;
    size_t conseq1 = 0;
    #endif
    
    for(;;)
    {
        int32_t bc = bufin[--i] << 24;
        
        uint32_t sh = 8;
        do
        {
            if(bc >= 0)
            {
                #ifdef LZ_DEBUG
                if(o >= 0x1000 && conseq1_curr)
                {
                    if(conseq1_max < conseq1_curr)
                        conseq1_max = conseq1_curr;
                    
                    conseq1_curr = 0;
                }
                
                conseq0 += 1;
                conseq0_curr += 1;
                #endif
                
                bufout[--o] = bufin[--i];
            }
            else
            {
                #ifdef LZ_DEBUG
                if(o >= 0x1000 && conseq0_curr)
                {
                    if(conseq0_max < conseq0_curr)
                        conseq0_max = conseq0_curr;
                    
                    conseq0_curr = 0;
                }
                
                conseq1 += 1;
                conseq1_curr += 1;
                #endif
                
                if(i < 2)
                {
                    #ifdef LZ_DEBUG
                    printf("lz err: %i < 2\n", i);
                    #endif
                    goto ret;
                }
                
                i -= 2;
                
                uint32_t dupoffs = bufin[i] | (bufin[i + 1] << 8);
                
                uint32_t duplen  = (dupoffs >> 12)   + 3;
                         dupoffs = (dupoffs & 0xFFF) + 3;
                
                if(o < duplen)
                {
                    #ifdef LZ_DEBUG
                    printf("duplen overflow %X < %X\n", o, duplen);
                    #endif
                    goto ret;
                }
                
                if((o + dupoffs) > outsize)
                {
                    #ifdef LZ_DEBUG
                    printf("offs overflow %X + %X >= %X\n", o, dupoffs, outsize);
                    #endif
                    goto ret;
                }
                
                o -= 1;
                
                const uint8_t* bufoutin = bufout + dupoffs;
                // Has to be copied backwards like this
                //  because of existing data
                do
                    bufout[o] = bufoutin[o];
                while(--duplen && --o);
            }
            
            bc <<= 1;
            
            if(o && i > e)
                continue;
            else
                goto ret;
        }
        while(--sh);
    }
    
    ret:
    
    #ifdef LZ_DEBUG
    /*if(conseq0_curr > conseq0_max)
        conseq0_max = conseq0_curr;
    
    if(conseq1_curr > conseq1_max)
        conseq1_max = conseq1_curr;
    */
    
    printf("lzdec ret %X %X %X\n", i, o, e);
    
    puts("conseq");
    printf("1 - %06X %X %X\n", conseq1, conseq1_max, conseq1_curr);
    printf("0 - %06X %X %X\n", conseq0, conseq0_max, conseq0_curr);
    #endif
    
    if(i == e)
    {
        for(i = 0; i < e; i += 1)
            bufout[i] = bufin[i];
        
        return outsize;
    }
    return outsize - o;
}
#endif
