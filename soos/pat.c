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
#include "trainer_bin.h"

//#define MEME_DEBUG

// only used for percentage display
const size_t PAT_HOLE_SIZE = 0x1E0; // run-once
const size_t HOK_HOLE_SIZE = 0x0F8; // frame trainers
const size_t RTC_HOLE_SIZE = 0x130; // rtcom

const size_t PAT_HOLE_ADDR = 0x130000 - 0x100; // not used for actual payload positioning

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
                
                #ifdef MEME_DEBUG
                printf("memesearch bit %X %X\n", i, j);
                for(j = 0; j != patsize; j++) printf("%02X", src[i + j]);
                puts("");
                for(j = 0; j != patsize; j++) printf("%02X", pat[j]);
                puts("");
                for(j = 0; j != patsize; j++) printf("%02X", bit[j]);
                puts("");
                #endif
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
                
                #ifdef MEME_DEBUG
                printf("memsearch nrm %X %X\n", i, j);
                for(j = 0; j != patsize; j++) printf("%02X", src[i + j]);
                puts("");
                for(j = 0; j != patsize; j++) printf("%02X", pat[j]);
                puts("");
                #endif
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

size_t pat_copyholerunonce(uint8_t* patchbuf, const color_setting_t* sets, size_t mask, size_t* outsize)
{
    if(!patchbuf)
        return mask & (PAT_REDSHIFT | PAT_WIDE | PAT_RELOC);
    
    uint16_t* tptr = (uint16_t*)patchbuf;
    
    uint32_t* reloclen = 0;
    
    uint32_t compress_o = 0;
    
    if((mask & PAT_REDSHIFT) && sets)
    {
        // copy Redshift decompressor code
        memcpy(tptr, redshift_bin, redshift_bin_size);
        tptr += (redshift_bin_size + 1) >> 1;
        
        // there is an ADR here in the decompressor
        uint32_t* compress_t = (uint32_t*)tptr;
        
        // the below code is taken from CTR_Redshift
        
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
        
        // compress the colorramp
        //  this compression algo abuses the fact
        //  that the colorramp is a ramp, and thus
        //  never goes down, allowing for
        //  extremely low compression ratio
        
        uint8_t prev[4];
        prev[0] = 0;
        prev[1] = 0;
        prev[2] = 0;
        prev[3] = 0;
        
        do
        {
            // it only goes to 3 instead of 4 to save bits
            // because the LCD is not transparent, save 256 bits
            // by not including the Alpha component
            for(j = 0; j != 3; j++)
            {
                uint8_t cur = c[i + (j << 8)] >> 8;
                if(cur < prev[j]) //never supposed to happen
                    puts("error wat");
                
                while(cur > prev[j]) // write increment bit while bigger
                {
                    prev[j]++;
                    compress_t[compress_o >> 5] |= 1 << (compress_o & 31);
                    compress_o++;
                }
                
                //write increment stop bit
                
                compress_t[compress_o >> 5] &= ~(1 << (compress_o & 31));
                compress_o++;
            }
        }
        while(++i);
        
        printf("compress_o bits=%i bytes=%X\n", compress_o, (compress_o + 7) >> 3);
        
        tptr += ((compress_o + 31) >> 5) << 1;
        
        // insert a security NOP just in case someting happens
        
        if(((uint8_t*)tptr - patchbuf) & 2) // how do you even
            *(tptr++) = 0x46C0; //NOP
        
        mask &= ~PAT_REDSHIFT;
    }
    
    if(mask & PAT_WIDE)
    {
        // these patches below modify the MTX hardware registers
        // DO NOT MODIFY THESE PATCHES
        
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
    
    // ===[Relocation]===
    
    if(mask & PAT_RELOC)
    {
        puts("Warning: Relocation is no longer supported. NO-OP");
        
        /*if(((uint8_t*)tptr - patchbuf) & 2)
            *(tptr++) = 0x46C0; //NOP
        
        memcpy(tptr, reloc_bin, reloc_bin_size);
        tptr += (reloc_bin_size + 1) >> 1;
        reloclen = ((uint32_t*)tptr) - 1;
        */
        
        if(outsize)
            outsize[1] = (size_t)((((uint8_t*)tptr - patchbuf) + 3) & ~3);
    }
    
    *(tptr++) = 0x4770; // BX LR
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
        
        printf("Runonce bytes used: %X/%X, %u%%\n", currbyte, maxbyte, (currbyte * (100 * 0x10000) / maxbyte) >> 16);
    }
    while(0);
    
    return mask;
}

size_t pat_copyholehook(uint8_t* patchbuf, size_t mask, size_t* outsize)
{
    if(!patchbuf)
        return mask & (PAT_HOLE | PAT_RTCOM);
    
    uint16_t* tptr = (uint16_t*)patchbuf;
    
    if(mask & PAT_HOLE)
    {
        memcpy(tptr, hole_bin, hole_bin_size);
        tptr += (hole_bin_size + 1) >> 1;
        
        if(((uint8_t*)tptr - patchbuf) & 2)
            *(tptr++) = 0x46C0; //NOP
    }
    
    if(mask & PAT_RTCOM)
    {
        puts("Warning: rtcom is not supported in the hook hole, NO-OP");
        
        // Only fix glitchy branch if HOLE or RELOC mess up the chain flow
        //if(!(mask & PAT_HOLE) != !(mask & PAT_RELOC))
        /*if(mask & PAT_HOLE)
        {
            puts("Glitchy branch workaround");
            *(tptr++) = 0x46C0; //NOP
            *(tptr++) = 0x46C0; //NOP
            *(tptr++) = 0x46C0; //NOP
            *(tptr++) = 0x46C0; //NOP
            
            if(((uint8_t*)tptr - patchbuf) & 2)
                *(tptr++) = 0x46C0; //NOP
        }
        
        memcpy(tptr, trainer_bin, trainer_bin_size);
        tptr += (trainer_bin_size + 1) >> 1;
        
        if(((uint8_t*)tptr - patchbuf) & 2)
            *(tptr++) = 0x46C0; //NOP*/
    }
    
    *(tptr++) = 0x4770; // BX LR
    *(tptr++) = 0x4770; // BX LR
    
    do
    {
        size_t currbyte = (((uint8_t*)tptr - patchbuf) + 3) & ~3;
        const size_t maxbyte = HOK_HOLE_SIZE;
        
        if(outsize)
            *outsize = currbyte;
        
        printf("Trainer bytes used: %X/%X, %u%%\n", currbyte, maxbyte, (currbyte * (100 * 0x10000) / maxbyte) >> 16);
    }
    while(0);
    
    return mask;
}

static void makeblT(uint8_t** buf, size_t to, size_t from)
{
    uint32_t toffs = (to - (from + 4)) & ~1;
    
    printf("from: %X to: %X diff: %X\n", from, to, toffs);
    
    uint8_t* res = *buf;
    
    *(res++) = (uint8_t)(toffs >> 12);
    *(res++) = (uint8_t)(((toffs >> 20) & 7) | 0xF0);
    *(res++) = (uint8_t)(toffs >> 1);
    *(res++) = (uint8_t)(((toffs >> 9) & 7) | 0xF8);
    
    
    printf("%02X %02X %02X %02X\n", res[-4 + 0], res[-4 + 1], res[-4 + 2], res[-4 + 3]);
    
    *buf = res;
}

size_t pat_apply(uint8_t* codecptr, size_t codecsize, const color_setting_t* sets, size_t mask)
{
    uint8_t* resptr = 0;
    size_t patmask = mask;
    
    // ===[Always patch]===
    
    /*resptr = memesearch(
        // unique instructions to send parameters to svcKernelSetState
        (const uint8_t[]){0x00, 0x22, 0x04, 0x20, 0x13, 0x46, 0x11, 0x46},
        0, codecptr, codecsize, 8
    );
    
    if(resptr)
    {
        puts("Patching out memory unmap");
        
        resptr[0x00] = 0xC0;
        resptr[0x01] = 0x46;
        resptr[0x02] = 0xC0;
        resptr[0x03] = 0x46;
        resptr[0x04] = 0xC0;
        resptr[0x05] = 0x46;
        resptr[0x06] = 0xC0;
        resptr[0x07] = 0x46;
        resptr[0x08] = 0xC0;
        resptr[0x09] = 0x46;
        resptr[0x0A] = 0xC0;
        resptr[0x0B] = 0x46;
        resptr[0x0C] = 0xC0;
        resptr[0x0D] = 0x46;
        resptr[0x0E] = 0xC0;
        resptr[0x0F] = 0x46;
        resptr[0x10] = 0xC0;
        resptr[0x11] = 0x46;
        resptr[0x12] = 0xC0;
        resptr[0x13] = 0x46;
    }*/
    
    // === remove opposing DPAD check
    
    if(patmask & PAT_HID)
    {
        resptr = memesearch(
            // bit stuff for removing opposing DPAD bits
            (const uint8_t[]){0x40, 0x20, 0x20, 0x40, 0x20, 0x21, 0x21, 0x40, 0x40, 0x00, 0x49, 0x08, 0x08, 0x43, 0x84, 0x43},
            0, codecptr, codecsize, 16
        );
        
        if(resptr)
        {
            puts("Patching DPAD check");
            
            resptr[0x0] = 0xC0;
            resptr[0x1] = 0x46;
            resptr[0x2] = 0xC0;
            resptr[0x3] = 0x46;
            resptr[0x4] = 0xC0;
            resptr[0x5] = 0x46;
            resptr[0x6] = 0xC0;
            resptr[0x7] = 0x46;
            resptr[0x8] = 0xC0;
            resptr[0x9] = 0x46;
            resptr[0xA] = 0xC0;
            resptr[0xB] = 0x46;
            resptr[0xC] = 0xC0;
            resptr[0xD] = 0x46;
            resptr[0xE] = 0xC0;
            resptr[0xF] = 0x46;
            
            mask &= ~PAT_HID;
        }
    }
    
    // === DMPGL patch for resolution changing
    
    if(patmask & PAT_WIDE)
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
            
            // these values below change the TEXTURE resolution, not the rendering resolution
            // DO NOT CHANGE
            
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
                    
                    // these two instructions set the RENDERING resolution
                    
                    resptr[0] = 0xFF;
                    resptr[1] = 0x22;
                    resptr[2] = 0x81;
                    resptr[3] = 0x32;
                    resptr[4] = 0x03;
                    resptr[5] = 0xE0;
                    resptr[6] = 0xC0;
                    resptr[7] = 0x46;
                    
                    resptr[8] = 0xA2; // skip one instruction
                    
                    // Do not clear the mask here, as extra patches are needed below
                    //mask &= ~PAT_WIDE;
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
                    
                    // Do not clear wide patch here
                    //mask &= ~PAT_WIDE;
                }
            }
        }
    }
    
    // ALL of these require hooking
    if(patmask & (PAT_HOLE | PAT_REDSHIFT | PAT_RTCOM | PAT_WIDE | PAT_RELOC | PAT_DEBUG))
    {
        // ==[ Alternative code spaces, unused for now ]==
        
        // Unknown
        //resptr = memesearch((const uint8_t[]){0x00, 0x29, 0x03, 0xD0, 0x40, 0x69, 0x08, 0x60, 0x01, 0x20, 0x70, 0x47, 0x00, 0x20, 0x70, 0x47},
        
        size_t addroffs = 0;
            
        // === find relocation offset offset
        uint8_t* res2ptr = memesearch(
            //this works on both AGBG and TwlBg
            (const uint8_t[]){0x7F, 0x00, 0x01, 0x00, 0x00, 0x00, 0xF0, 0x1E},
            0, codecptr, codecsize, 8
        );
        
        if(res2ptr)
        {
            // fileoffs + addroffs == vaddr
            //   01FF00h+ addroffs == 100000h
            //   400000h-   31FF00h== 0E0100h
            addroffs = 0x400000 - *(uint32_t*)(res2ptr + 8);
        }
        else
        {
            puts("Warning: failed to find relocation offset, some patches won't apply");
        }
        
        // Copy runonce blob first, if needed
        if(addroffs && pat_copyholerunonce(0, 0, patmask, 0))
        {
            resptr = memesearch(
                // Start of the unused(?) MTX driver blob
                (const uint8_t[]){0x00, 0x23, 0x49, 0x01, 0xF0, 0xB5, 0x16, 0x01},
                0, codecptr, codecsize, 8
            );
            
            if(resptr)
            {
                // == post-init pre-unmap debug print hook (white screen before svcKernelSetState(4))
                // used for runonce purposes
                res2ptr = !agbg ?
                    memesearch
                    (
                        (const uint8_t[]){0x00, 0x28, 0x01, 0xDB, 0x11, 0x98, 0x40, 0x68, 0x58, 0xA1, 0x13, 0xF0, 0x8B, 0xF9},
                        (const uint8_t[]){0x00, 0x00, 0x07, 0x00, 0xFF, 0x00, 0xC0, 0x07, 0xFF, 0x00, 0xFF, 0x07, 0xFF, 0x07},
                        codecptr, codecsize,
                        14
                    )
                    :
                    memesearch
                    (
                        (const uint8_t[]){0x30, 0x46, 0x10, 0x38, 0xC0, 0x46, 0xC0, 0x46},
                        0, codecptr, codecsize, 8
                    );
                
                
                if(res2ptr && (!agbg || !((res2ptr - codecptr) & 3)))
                {
                    printf("runonce hook %X\n", (res2ptr - codecptr) + addroffs);
                    
                    res2ptr[0] = 0xC0;
                    res2ptr[1] = 0x46;
                    res2ptr[2] = 0xC0;
                    res2ptr[3] = 0x46;
                    res2ptr[4] = 0xC0;
                    res2ptr[5] = 0x46;
                    res2ptr[6] = 0xC0;
                    res2ptr[7] = 0x46;
                    
                    if(!agbg)
                    {
                        //there is more space in TwlBg to clear
                        res2ptr[ 8] = 0xC0;
                        res2ptr[ 9] = 0x46;
                        res2ptr[10] = 0xC0;
                        res2ptr[11] = 0x46;
                        res2ptr[12] = 0xC0;
                        res2ptr[13] = 0x46;
                    }
                    
                    if((res2ptr - codecptr) & 2)
                    {
                        *(res2ptr++) = 0xC0;
                        *(res2ptr++) = 0x46;
                    }
                    
                    if((resptr - codecptr) & 2)
                    {
                        *(resptr++) = 0xC0;
                        *(resptr++) = 0x46;
                    }
                    
                    // [0] - total hole usage
                    // [1] - offset from hole start to trainer code (per-frame)
                    size_t copyret[2];
                    copyret[1] = 0; // has to be initialized!
                    
                    // do not clear PAT_RELOC because it's cleared below
                    mask &= pat_copyholerunonce(resptr, sets, patmask & ~PAT_RELOC, copyret) & ~PAT_RELOC;
                    
                    uint32_t sraddr = (res2ptr - codecptr) + addroffs;
                    uint32_t toaddr = (resptr - codecptr) + 0x300000;
                    
                    // BL debughook --> runonce
                    makeblT(&res2ptr, toaddr, sraddr);
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
                puts("Can't apply runonce patches due to missing hole");
            }
        }
        else if(!addroffs)
        {
            puts("Can't apply runonce patches due to missing relocation offset");
        }
        
        if(addroffs && pat_copyholehook(0, patmask, 0))
        {
            // Frame hook ALWAYS has to be hooked, no matter if rtcom is on,
            //  or if there are any patches present. Either of them always does.
            do
            {
                // This is a really hacky way to find an unused initializer table
                //  and safely overwrite them without overwriting an used function in the middle
                
                const uint8_t spat[] = {0x04, 0x48, 0x00, 0x93, 0x13, 0x46, 0x0A, 0x46, 0x00, 0x68, 0x21, 0x46};
                const uint8_t smat[] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                
                resptr = memesearch(spat, smat,
                    codecptr, codecsize, sizeof(spat));
                
                if(resptr)
                {
                    // This is a two-pass pattern
                    resptr = memesearch(spat, smat,
                        resptr + 1, codecsize - (codecptr - resptr), sizeof(spat));
                }
                
                if(resptr)
                {
                    printf("pat2 at %X\n", (resptr - codecptr) + addroffs);
                    
                    resptr += 0x38; // Skip used function blob
                    
                    // Align search ptr (skip const data and find real func start)
                    if(resptr[1] != 0x48 || !resptr[3])
                    {
                        resptr += 4;
                        if(resptr[1] != 0x48 || !resptr[3])
                        {
                            printf("Invalid pattern match at %X\n", (resptr - codecptr) + addroffs);
                            resptr = 0;
                        }
                    }
                }
            }
            while(0);
            
            // Make sure we found the right blob at the very start
            if(resptr && (resptr - codecptr) < 0x20000)
            {
                printf("Frame hook target %X\n", (resptr - codecptr) + addroffs);
            
                if((resptr - codecptr) & 2) //must be 4-aligned for const pools
                {
                    *(resptr++) = 0xC0;
                    *(resptr++) = 0x46;
                }
                
                // Find frame hook
                res2ptr = !agbg ?
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
                
                if(res2ptr)
                {
                    // the function is below the consts the memesearch pattern searches for
                    res2ptr += 0x10;
                    if(!agbg)
                        res2ptr += 6; // 0x16
                    
                    printf("Frame hook patch  %X\n", res2ptr-codecptr+addroffs);
                    
                    // MOV(S) r0, #0 around the BL
                    res2ptr[0] = 0x00;
                    res2ptr[1] = 0x20;
                    
                    res2ptr[2] = 0x00;
                    res2ptr[3] = 0x20;
                    res2ptr[4] = 0x00;
                    res2ptr[5] = 0x20;
                    
                    res2ptr[6] = 0x00;
                    res2ptr[7] = 0x20;
                    res2ptr[8] = 0x00;
                    res2ptr[9] = 0x20;
                    
                    if((res2ptr - codecptr) & 3)
                    {
                        *(res2ptr++) = 0xC0;
                        *(res2ptr++) = 0x46;
                        printf("Aligned code to   %X\n", res2ptr-codecptr+addroffs);
                    }
                    
                    printf("Unused init blob vaddr: %X\n", (resptr - codecptr) + addroffs);
                    
                    // destination to baked trainer init code (in 3xxxxx region, runonce)
                    uint32_t sraddr = (res2ptr - codecptr) + addroffs;
                    uint32_t toaddr = (resptr - codecptr) + addroffs;
                    
                    // BL framehook --> rtcom
                    makeblT(&res2ptr, toaddr, sraddr);
                    
                    // rtcom has its own dedicated hole
                    if(patmask & PAT_RTCOM)
                    {
                        memcpy(resptr, trainer_bin, trainer_bin_size);
                        
                        printf("rt bytes used: %X/%X, %u%%\n", trainer_bin_size + 4, RTC_HOLE_SIZE,
                            ((trainer_bin_size + 8) * (100 * 0x10000) / RTC_HOLE_SIZE) >> 16);
                        
                        resptr += (trainer_bin_size + 3) & ~3;
                        
                        mask &= ~PAT_RTCOM;
                    }
                    
                    uint8_t* mtxblob = 0;
                    
                    if(pat_copyholehook(0, patmask & ~PAT_RTCOM, 0))
                    {
                        do
                        {
                            //10189C - 1018C4 - 319C0C
                            // == frame hook hole
                            const uint8_t* patn = (const uint8_t[])
                            { 0x02, 0x48, 0x10, 0xB5, 0x00, 0x68, 0x18, 0xF2, 0x75, 0xF9, 0x10, 0xBD, 0x18, 0x6F, 0x12, 0x00, 0x01, 0x46 };
                            const uint8_t* patb = (const uint8_t[])
                            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x07, 0xFF, 0x07, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00 };
                            
                            const size_t patl = 18;
                            
                            mtxblob = memesearch(patn, patb, codecptr, codecsize, patl);
                            if(mtxblob)
                                mtxblob = memesearch(patn, patb, mtxblob + 1, codecsize - (mtxblob - codecptr), patl);
                        }
                        while(0);
                        
                        if(mtxblob)
                        {
                            if((mtxblob - codecptr) & 2)
                            {
                                *(mtxblob++) = 0xC0;
                                *(mtxblob++) = 0x46;
                            }
                            
                            puts("Hooking rtcom --> extrapatches");
                            
                            mask &= pat_copyholehook(mtxblob, mask, 0);
                            
                            // PUSH {r0, LR}
                            *(resptr++) = 0x01;
                            *(resptr++) = 0xB5;
                            // NOP
                            *(resptr++) = 0xC0;
                            *(resptr++) = 0x46;
                            
                            sraddr = (resptr - codecptr) + addroffs;
                            toaddr = (mtxblob - codecptr) + addroffs;
                            
                            // BL rtcom_end --> extrablob
                            makeblT(&resptr, toaddr, sraddr);
                            
                            // NOP
                            *(resptr++) = 0xC0;
                            *(resptr++) = 0x46;
                            // POP {r0, PC}
                            *(resptr++) = 0x01;
                            *(resptr++) = 0xBD;
                            
                            // ==[ DO NOT USE ]==
                            /*resptr = memesearch(
                                (const uint8_t[]){0x03, 0x21, 0x1F, 0x20, 0x49, 0x05, 0x00, 0x06, 0x10, 0xB5},
                                0, codecptr, codecsize, 10
                            );
                            if(resptr)
                            {
                                puts("Patching anti-trainer");
                                
                                resptr[0] = 0x70;
                                resptr[1] = 0x47;
                                resptr[2] = 0x70;
                                resptr[3] = 0x47;
                                resptr[4] = 0x70;
                                resptr[5] = 0x47;
                                resptr[6] = 0x70;
                                resptr[7] = 0x47;
                            }*/
                        }
                        else
                        {
                            puts("Failed to find Hole2");
                        }
                    }
                    
                    // BX LR; BX LR;
                    *(resptr++) = 0x70;
                    *(resptr++) = 0x47;
                    *(resptr++) = 0x70;
                    *(resptr++) = 0x47;
                }
                else
                {
                    puts("Can't find frame hook");
                }
            }
            else
            {
                puts("Can't find unused init blob");
            }
        }
    }
    
    return mask & patmask;
}
