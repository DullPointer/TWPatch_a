#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pat.h"

typedef uint8_t u8;
typedef uint32_t u32;

#include "redshift_bin.h"
#include "hole_bin.h"
#include "reloc_bin.h"
#include "agb_wide_bin.h"
#include "twl_wide_bin.h"

const size_t PAT_HOLE_SIZE = 0x1E0; // actually 0x1E8, but unsafe

extern int agbg;

__attribute__((optimize("Ofast"))) void* memesearch(const void* patptr, const void* bitptr, const void* searchptr, size_t searchlen, size_t patsize)
{
    const uint8_t* pat = patptr;
    const uint8_t* bit = bitptr;
    const uint8_t* src = searchptr;
    
    size_t i = 0;
    size_t j = 0;
    
    searchlen -= patsize;
    
    if(bit)
    {
        do
        {
            if((src[i + j] & ~bit[j]) == (pat[j] & ~bit[j]))
            {
                if(++j != patsize)
                    continue;
                
                printf("memesearch bit %X %X\n", i, j);
                for(j = 0; j != patsize; j++) printf("%02X", src[i + j]);
                puts("");
                for(j = 0; j != patsize; j++) printf("%02X", pat[j]);
                puts("");
                for(j = 0; j != patsize; j++) printf("%02X", bit[j]);
                puts("");
                return src + i;
            }
            
            ++i;
            j = 0;
            
            continue;
        }
        while(i != searchlen);
    }
    else
    {
        do
        {
            if(src[i + j] == pat[j])
            {
                if(++j != patsize)
                    continue;
                
                printf("memesearch nrm %X %X\n", i, j);
                for(j = 0; j != patsize; j++) printf("%02X", src[i + j]);
                puts("");
                for(j = 0; j != patsize; j++) printf("%02X", pat[j]);
                puts("");
                return src + i;
            }
            
            ++i;
            j = 0;
            
            continue;
        }
        while(i != searchlen);
    }
    
    return 0;
}

size_t pat_copyhole(uint8_t* patchbuf, const color_setting_t* sets, size_t mask, size_t* outsize)
{
    uint16_t* tptr = (uint16_t*)patchbuf;
    
    uint32_t* reloclen = 0;
    
    uint32_t compress_o = 0;
    
    if((mask & PAT_REDSHIFT) && sets)
    {
        memcpy(tptr, redshift_bin, redshift_bin_size);
        tptr += (redshift_bin_size + 1) >> 1;
        
        uint32_t* compress_t = (uint32_t*)tptr;
        
        uint16_t c[0x300];
        uint8_t i = 0;
        uint8_t j = 0;
        
        do
        {
            *(c + i + 0x000) = i | (i << 8);
            *(c + i + 0x100) = i | (i << 8);
            *(c + i + 0x200) = i | (i << 8);
            //*(c + i + 0x300) = 0;
        }
        while(++i);
        
        colorramp_fill(c + 0x000, c + 0x100, c + 0x200, 0x100, sets);
        
        uint8_t prev[4];
        prev[0] = 0;
        prev[1] = 0;
        prev[2] = 0;
        prev[3] = 0;
        
        do
        {
            for(j = 0; j != 3; j++)
            {
                uint8_t cur = c[i + (j << 8)] >> 8;
                if(cur < prev[j])
                    puts("error wat");
                
                while(cur > prev[j])
                {
                    prev[j]++;
                    compress_t[compress_o >> 5] |= 1 << (compress_o & 31);
                    compress_o++;
                }
                
                compress_t[compress_o >> 5] &= ~(1 << (compress_o & 31));
                compress_o++;
            }
        }
        while(++i);
        
        printf("compress_o bits=%i bytes=%X\n", compress_o, (compress_o + 7) >> 3);
        
        tptr += ((compress_o + 31) >> 5) << 1;
        
        if(((uint8_t*)tptr - patchbuf) & 2) // how do you even
            *(tptr++) = 0x46C0; //NOP
        
        mask &= ~PAT_REDSHIFT;
    }
    
    /*
    if(mask & PAT_WIDE)
    {
        if(!agbg)
        {
            memcpy(tptr, twl_wide_bin, twl_wide_bin_size);
            tptr += (twl_wide_bin_size + 1) >> 1;
        }
        else
        {
            memcpy(tptr, agb_wide_bin, agb_wide_bin_size);
            tptr += (agb_wide_bin_size + 1) >> 1;
        }
        
        if(((uint8_t*)tptr - patchbuf) & 2) // how do you even
            *(tptr++) = 0x46C0; //NOP
        
        mask &= ~PAT_WIDE;
    }
    */
    
    if(mask & PAT_RELOC)
    {
        memcpy(tptr, reloc_bin, reloc_bin_size);
        tptr += (reloc_bin_size + 1) >> 1;
        reloclen = ((uint32_t*)tptr) - 1;
    }
    
    if(mask & PAT_HOLE)
    {
        memcpy(tptr, hole_bin, hole_bin_size);
        tptr += (hole_bin_size + 1) >> 1;
        
        if(((uint8_t*)tptr - patchbuf) & 2)
            *(tptr++) = 0x46C0; //NOP
    }
    
    *(tptr++) = 0x4670; // MOV r0, LR
    *(tptr++) = 0x3004; // ADD r0, #4
    *(tptr++) = 0x4686; // MOV LR, r0
    *(tptr++) = 0x4770; // BX LR
    
    do
    {
        size_t currbyte = (((uint8_t*)tptr - patchbuf) + 3) & ~3;
        const size_t maxbyte = PAT_HOLE_SIZE;
        
        if(outsize)
            *outsize = currbyte;
        
        if(reloclen)
        {
            *reloclen = (((uint8_t*)tptr - (uint8_t*)reloclen) + 3) & ~3;
            printf("Relocation %X\n", *reloclen);
        }
        
        printf("Bytes used: %X/%X, %u%%\n", currbyte, maxbyte, (currbyte * (100 * 0x10000) / maxbyte) >> 16);
    }
    while(0);
    
    return mask;
}

size_t pat_apply(uint8_t* codecptr, size_t codecsize, const color_setting_t* sets, size_t mask)
{
    uint8_t* resptr = 0;
    
    // === DMPGL patch for resolution changing
    
    if(mask & PAT_WIDE)
    {
        // (const uint8_t[]){0xD2, 0x4C, 0x00, 0x26, 0x00, 0x28}, DMA PATCH
        resptr = memesearch
        (
            // == DMPGL width patch
            !agbg ?
                (const uint8_t[]){0xFF, 0x24, 0x41, 0x34, 0xF0, 0x26}
            :
                (const uint8_t[]){0xFF, 0x25, 0x69, 0x35, 0xF0, 0x24}
                ,
            0, codecptr, codecsize, 6);
        if(resptr)
        {
            puts("Applying wide patch");
            if(!agbg)
                resptr[2] += (384 - 320); //lol
            else
                resptr[2] += (400 - 360);
            
            if(!agbg)
            {
                // == ??? TODO document
                resptr = memesearch(
                    (const uint8_t[]){0x00, 0x2A, 0x04, 0xD0, 0x23, 0x61, 0xA1, 0x80, 0xA1, 0x60},
                    0, codecptr, codecsize, 10);
                if(resptr)
                {
                    puts("Applying DMPGL patch");
                    
                    resptr[0] = 0xFF;
                    resptr[1] = 0x22;
                    resptr[2] = 0x81;
                    resptr[3] = 0x32;
                    resptr[4] = 0x03;
                    resptr[5] = 0xE0;
                    resptr[6] = 0xC0;
                    resptr[7] = 0x46;
                    
                    resptr[8] = 0xA2;
                    
                    mask &= ~PAT_WIDE;
                }
            }
            else
            {
                // == ??? TODO DOCUMENT
                resptr = memesearch(
                    //(const uint8_t[]){0xFF, 0x21, 0x41, 0x31, 0xA3, 0x60, 0xE1, 0x60},
                    (const uint8_t[]){0x00, 0x01, 0x00, 0x00, 0x68, 0x01, 0x00, 0x00},
                    0, codecptr, codecsize, 8);
                if(resptr)
                {
                    puts("Applying DMPGL patch");
                    //resptr[2] += (400 - 360);
                    resptr[4] += (400 - 360);
                    
                    mask &= ~PAT_WIDE;
                }
            }
        }
    }
    
    // === unused MTX driver code
    
    if(mask & (PAT_HOLE | PAT_REDSHIFT)) // ALL of these require the unused driver code space
    {
        //resptr = memesearch((const uint8_t[]){0x00, 0x29, 0x03, 0xD0, 0x40, 0x69, 0x08, 0x60, 0x01, 0x20, 0x70, 0x47, 0x00, 0x20, 0x70, 0x47},
        resptr = memesearch((const uint8_t[]){0x00, 0x23, 0x49, 0x01, 0xF0, 0xB5, 0x16, 0x01},
            0, codecptr, codecsize, 8);
        if(resptr && (resptr - codecptr) < 0x1EE00)
        {
            puts("Overwriting unused function");
            
            size_t it = 0;
            
            // make sure the code crashes when a fuckup happens
            do
            {
                //resptr[it++] = 0xFE;
                //resptr[it++] = 0xE7;
                resptr[it++] = 0xFF;
                resptr[it++] = 0xBE;
            }
            while(it != PAT_HOLE_SIZE);
            
            if((resptr - codecptr) & 2) //must be 4-aligned for const pools
            {
                *(resptr++) = 0xC0;
                *(resptr++) = 0x46;
            }
            
            // == post-init pre-unmap debug print hook
            uint8_t* res2ptr= !agbg ? memesearch
            (
                (const uint8_t[]){0x00, 0x28, 0x01, 0xDB, 0x11, 0x98, 0x40, 0x68, 0x58, 0xA1, 0x13, 0xF0, 0x8B, 0xF9},
                (const uint8_t[]){0x00, 0x00, 0x07, 0x00, 0xFF, 0x00, 0xC0, 0x07, 0xFF, 0x00, 0xFF, 0x07, 0xFF, 0x07},
                codecptr, codecsize,
                14
            )
            :
            memesearch
            (
                //(const uint8_t[]){0x6C, 0xA1, 0x30, 0x46, 0x12, 0xF0, 0x59, 0xFF},
                //(const uint8_t[]){0xFF, 0x00, 0x30, 0x46, 0xFF, 0x07, 0xFF, 0x07},
                (const uint8_t[]){0x30, 0x46, 0x10, 0x38, 0xC0, 0x46, 0xC0, 0x46},
                0, codecptr, codecsize, 8
            );
            
            printf("res2ptr %X %X\n", res2ptr, res2ptr - codecptr);
            
            if(res2ptr && (!agbg || !((res2ptr - codecptr) & 3)))
            {
                if(!agbg)
                {
                    //there is more space in TwlBg, so NOPE it out
                    res2ptr[ 8] = 0xC0;
                    res2ptr[ 9] = 0x46;
                    res2ptr[10] = 0xC0;
                    res2ptr[11] = 0x46;
                    res2ptr[12] = 0xC0;
                    res2ptr[13] = 0x46;
                    
                    if((res2ptr - codecptr) & 3) //must be 4-aligned for Thumb ADR
                    {
                        *(res2ptr++) = 0xC0;
                        *(res2ptr++) = 0x46;
                    }
                }
                
                // LDR && BLX PC
                *(res2ptr++) = 0x00;
                *(res2ptr++) = 0x49;
                *(res2ptr++) = 0x88;
                *(res2ptr++) = 0x47;
                
                // THIS HAS TO BE IN THE 300000h REGION for the LR fix to work!
                uint32_t absaddr = (((resptr - codecptr) + 0x300000) & ~1) | 1; // is Thumb
                *(res2ptr++) = absaddr;
                *(res2ptr++) = absaddr >> 8;
                *(res2ptr++) = absaddr >> 16;
                *(res2ptr++) = absaddr >> 24;
                
                res2ptr = resptr; // hole offset
                resptr = 0;
                
                // === find relocation offset offset
                if(!(~mask & (PAT_RELOC | PAT_HOLE))) //don't relocate if PAT_HOLE is not set
                {
                    //this works on both AGBG and TwlBg
                    resptr = memesearch(
                        (const uint8_t[]){0x7F, 0x00, 0x01, 0x00, 0x00, 0x00, 0xF0, 0x1E},
                        0, codecptr, codecsize, 8
                    );
                }
                
                // do not clear PAT_RELOC because it's cleared below
                mask &= ~(pat_copyhole(res2ptr, sets, mask, 0) & ~(PAT_RELOC)); //TODO: check out size
                
                if(resptr)
                {
                    // fileoffs + addroffs == vaddr
                    size_t addroffs = 0x400000 - *(uint32_t*)(resptr + 8);
                    
                    // == find dummy error code return function called from MainLoop
                    res2ptr = memesearch(
                        //same for both AGBG and TwlBg
                        (const uint8_t[]){0x00, 0x48, 0x70, 0x47, 0x00, 0x38, 0x40, 0xC9},
                        0, codecptr, codecsize, 8
                    );
                    
                    // == find MainLoop function
                    resptr = !agbg ?
                        memesearch
                        (
                            // ???
                            (const uint8_t[]){0x80, 0x71, 0xDF, 0x0F, 0x18, 0x09, 0x12, 0x00},
                            (const uint8_t[]){0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00},
                            codecptr, codecsize, 8
                        )
                    :
                        memesearch
                        (
                            //TODO: relies on magic DCW 0xFFFF
                            (const uint8_t[]){0xFF, 0xFF, 0x0B, 0x48, 0x10, 0xB5, 0x11, 0xF0},
                            (const uint8_t[]){0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00},
                            codecptr, codecsize, 8
                        )
                    ;
                    
                    if(res2ptr && resptr)
                    {
                        // the function is below the consts the memesearch is for
                        resptr += 0x10;
                        if(!agbg)
                            resptr += 6; // 0x16
                        
                        printf("Frame hook function %X %X %08X\n", res2ptr, res2ptr - codecptr, res2ptr-codecptr+addroffs);
                        printf("Frame hook patch    %X %X %08X\n", resptr, resptr - codecptr, resptr-codecptr+addroffs);
                        
                        // MOV(S) r0, #0 around the BL
                        resptr[0] = 0x00;
                        resptr[1] = 0x20;
                        
                        //resptr[2] = 0x00;
                        //resptr[3] = 0x20;
                        //resptr[4] = 0x00;
                        //resptr[5] = 0x20;
                        
                        resptr[6] = 0x00;
                        resptr[7] = 0x20;
                        resptr[8] = 0x00;
                        resptr[9] = 0x20;
                        
                        res2ptr = resptr;
                        
                        if((res2ptr - codecptr) & 3) //must be 4-aligned for Thumb ADR
                        {
                            *(res2ptr++) = 0xC0;
                            *(res2ptr++) = 0x46;
                            printf("Aligned code to %X\n", res2ptr-codecptr+addroffs);
                        }
                        
                        *(res2ptr++) = 0x00;
                        *(res2ptr++) = 0x49;
                        *(res2ptr++) = 0x88;
                        *(res2ptr++) = 0x47;
                        
                        absaddr = 0x200000 | 1; // is Thumb
                        
                        *(res2ptr++) = absaddr;
                        *(res2ptr++) = absaddr >> 8;
                        *(res2ptr++) = absaddr >> 16;
                        *(res2ptr++) = absaddr >> 24;
                        
                        mask &= ~PAT_RELOC;
                        
                        resptr = memesearch(
                            (const uint8_t[]){0x03, 0x21, 0x1F, 0x20, 0x49, 0x05, 0x00, 0x06},
                            0, codecptr, codecsize,
                            8
                        );
                        if(resptr)
                        {
                            puts("Patching anti-trainer");
                            
                            resptr[0] = 0x70;
                            resptr[1] = 0x47;
                            resptr[2] = 0x70;
                            resptr[3] = 0x47;
                        }
                    }
                }
                else
                {
                    puts("Can't relocate frame hook");
                }
            }
            else
            {
                if(!res2ptr)
                    puts("Can't hook init function");
                else if(agbg)
                    puts("Unlucky misalignment (AGBG)");
                else
                    puts("sumimasen nan fakku how the hell did you even get here");
            }
        }
        else
        {
            puts("Can't find unused MTX driver code");
        
        }
    }
    
    return mask;
}
