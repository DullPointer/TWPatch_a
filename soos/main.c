/*
    LgyPatPat - TwlBg Matrix hardware matrix patcher
    Copyright (C) 2019 Sono (https://github.com/SonoSooS)
    All Rights Reserved
*/

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "dummy_bin.h"
#include "hole_bin.h"
#include "redshift_bin.h"
#include "testimage_raw.h"
#include "lz.h"
#include "krn.h"

#include "redshift_all.h"

static int agbg = 0;

static void* LoadSection0(size_t* outsize)
{
    u32 apath[4];
    apath[0] = 0x20000002 | (1 << (8 | agbg));
    apath[1] = 0x00040138;
    apath[2] = 0;
    apath[3] = 0;
    
    char fpath[20];
    memset(fpath, 0, sizeof(fpath));
    fpath[ 8] = 2;
    fpath[12] = '.';
    fpath[13] = 'f';
    fpath[14] = 'i';
    fpath[15] = 'r';
    fpath[16] = 'm';
    
    FS_Path spath;
    spath.type = PATH_BINARY;
    spath.size = sizeof(apath);
    spath.data = apath;
    
    FS_Path lpath;
    lpath.type = PATH_BINARY;
    lpath.size = sizeof(fpath);
    lpath.data = fpath;
    
    size_t codesize;
    size_t offs = 0;
    void* ret = 0;
    u32 rd = 0;
    
    Handle h = 0;
    Result res = FSUSER_OpenFileDirectly(&h, ARCHIVE_SAVEDATA_AND_CONTENT, spath, lpath, FS_OPEN_READ, 0);
    if(res < 0)
    {
        //printf("OpenFileDirectly fail %08X\n", res);
        apath[0] &= ~0x20000000;
        res = FSUSER_OpenFileDirectly(&h, ARCHIVE_SAVEDATA_AND_CONTENT, spath, lpath, FS_OPEN_READ, 0);
    }
    if(res < 0)
    {
        loadsd:
        if(agbg)
        {
            puts("Please re-launch with no buttons pressed");
            return 0;
        }
        
        //asm volatile("NOP");
        
        uint8_t buf[0x200];
        //printf("OpenFileDirectly fail %08X\n", res);
        //puts("Can't open TWL_FIRM from NAND");
        FILE* f = fopen("/luma/section0.bin", "rb");
        if(!f)
        {
            puts("Can't open /luma/section0.bin");
            puts("Please mount app file from CTRNAND");
            puts("/title/00040138/?0000102/");
            puts("    ########.app in GM9 where");
            puts("  ? is 2 on new3DS and 0 on old3DS");
            puts("  # is the lowest on the file list");
            puts("and save exefs.bin as");
            puts(" /luma/section0.bin on your SDCard.");
            puts("!! FILE IS COPYRIGHTED ! DO NOT SHARE !!");
            return 0;
        }
        
        if(fread(buf, 0x200, 1, f) == 1 && *(const uint32_t*)(buf + 0x100) == *(const uint32_t*)"NCCH") goto nasd;
        if(fread(buf, 0x200, 1, f) == 1 && *(const uint32_t*)(buf + 0x100) == *(const uint32_t*)"NCCH") goto nasd;
        if(fread(buf, 0x200, 1, f) == 1 && *(const uint32_t*)(buf + 0x100) == *(const uint32_t*)"NCCH") goto nasd;
        if(fread(buf, 0x200, 1, f) == 1 && *(const uint32_t*)(buf + 0x100) == *(const uint32_t*)"NCCH") goto nasd;
        if(fread(buf, 0x200, 1, f) == 1 && *(const uint32_t*)(buf + 0x100) == *(const uint32_t*)"NCCH") goto nasd;
        if(fread(buf, 0x200, 1, f) == 1 && *(const uint32_t*)(buf + 0x100) == *(const uint32_t*)"NCCH") goto nasd;
        if(fread(buf, 0x200, 1, f) == 1 && *(const uint32_t*)(buf + 0x100) == *(const uint32_t*)"NCCH") goto nasd;
        if(fread(buf, 0x200, 1, f) == 1 && *(const uint32_t*)(buf + 0x100) == *(const uint32_t*)"NCCH") goto nasd;
        
        puts("/luma/section0.bin is not NCCH or FIRM");
        fclose(f);
        return 0;
        
        nasd:
        
        codesize = *(const uint32_t*)(buf + 0x104) << 9;
        ret = malloc((codesize + 0x1FF) & ~0x1FF);
        if(!ret)
        {
            printf("Corrupt NCCH size %X\n", *(const uint32_t*)(buf + 0x104));
            fclose(f);
            return 0;
        }
        
        memcpy(ret, buf, 0x200);
        
        if(fread(buf, 0x200, 1, f) != 1 || *(const uint64_t*)buf != *(const uint64_t*)"TwlBg\0\0")
        {
            puts("/luma/section0.bin is not TwlBg");
            fclose(f);
            return 0;
        }
        
        memcpy(ret + 0x200, buf, 0x200);
        
        if(fread(ret + 0x400, codesize - 0x400, 1, f) != 1)
        {
            puts("/luma/section0.bin can't read");
            fclose(f);
            return 0;
        }
        
        fclose(f);
        
        *outsize = codesize;
        return ret;
    }
    
    u64 fsfs = 0;
    if(FSFILE_GetSize(h, &fsfs) <0)
    {
        //puts("Can't get FIRM size");
        goto closeded;
    }
    
    //printf("size %llX\n", fsfs);
    
    ret = malloc((size_t)fsfs);
    if(!ret)
    {
        //puts("Out of memory");
        goto closeded;
    }
    
    if(FSFILE_Read(h, &rd, 0, ret, (u32)fsfs) < 0 || rd != fsfs)
    {
        //puts("Can't read FIRM");
        goto closeded;
    }
    
    FSFILE_Close(h);
    h = 0;
    
    if(*(const uint64_t*)(ret) != *(const uint64_t*)"FIRM\0\0\0")
    {
        //puts("TWL_FIRM is not a FIRM");
        goto closeded;
    }
    
    for(offs = 0x200; offs != 0x1200; offs += 0x200)
    {
        if(*(const uint32_t*)((ret + offs) + 0x100) == *(const uint32_t*)"NCCH")
            goto asd;
    }
    
    //puts("Can't find TwlBg in FIRM");
    goto closeded;
    
    asd:
    
    //memcpy(buf, ret + offs, 0x200);
    //codesize = *(const uint32_t*)(buf + 0x104) << 9;
    codesize = *(const uint32_t*)(ret + offs + 0x104) << 9;
    //printf("Codesize %04X %X %llX\n", *(const uint32_t*)(ret + offs + 0x104), codesize, fsfs);
    /*free(ret);
    ret = malloc(offs + ((codesize + 0x1FF) & ~0x1FF));
    if(!ret)
    {
        printf("Corrupt NCCH size %X\n", *(const uint32_t*)(buf + 0x104));
        goto closeded;
    }*/
    
    //memcpy(ret, buf, 0x200);
    
    if((*(const uint64_t*)(ret + offs + 0x200) ^ *(const uint64_t*)"TwlBg\0\0") & ~0xE1015)
    {
        //puts("NCCH in FIRM is not TwlBg");
        goto closeded;
    }
    
    /*memcpy(ret + 0x200, buf, 0x200);
    
    offs += 0x200;
    
    if(FSFILE_Read(h, &rd, offs, ret + 0x400, codesize - 0x400) < 0 || rd != (codesize - 0x400))
    {
        puts("Truncated TwlBg in FIRM");
        goto closeded;
    }*/
    
    //FSFILE_Close(h);
    
    *outsize = codesize;
    void* tst = malloc(codesize);
    if(!tst)
    {
        free(ret);
        return 0;
    }
    memcpy(tst, ret + offs, codesize);
    free(ret);
    return tst;
    //return ret + offs; //oof
    
    closeded:
    
    if(h)
        FSFILE_Close(h);
    h = 0;
    
    goto loadsd;
    //return 0;
}

#include "krnlist_all.h"

static inline void putpixel(uint32_t* fb, uint32_t px, uint32_t x, uint32_t y)
{
    fb[(x * 240) + (239 - y)] = __builtin_bswap32(px);
}

#include "agb_wide_bin.h"
#include "twl_wide_bin.h"
#include "reloc_bin.h"

static void calc_redshift(uint8_t* patchbuf, const color_setting_t* sets)
{
    uint16_t* tptr = (uint16_t*)patchbuf;
    
    uint32_t* reloclen = 0;
    
    if(!agbg)
    {
        memcpy(tptr, reloc_bin, reloc_bin_size);
        tptr += (reloc_bin_size + 1) >> 1;
        reloclen = ((uint32_t*)tptr) - 1;
    }
    
    /*
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
    
    if(((uint8_t*)tptr - patchbuf) & 2)
        *(tptr++) = 0x46C0; //NOP
    */
    
    memcpy(tptr, hole_bin, hole_bin_size);
    tptr += (hole_bin_size + 1) >> 1;
    
    if(((uint8_t*)tptr - patchbuf) & 2)
        *(tptr++) = 0x46C0; //NOP
    
    uint32_t compress_o = 0;
    
    if(sets)
    {
        memcpy((uint8_t*)tptr, redshift_bin, redshift_bin_size - 8);
        tptr += (redshift_bin_size - 7) >> 1;
        
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
        
        tptr += (compress_o + 15) >> 4;
    }
    else
    {
        *(tptr++) = 0x4770; // BX LR
        *(tptr++) = 0x4770; // BX LR
    }
    
    do
    {
        size_t currbyte = (((uint8_t*)tptr - patchbuf) + 3) & ~3;
        const size_t maxbyte = 0x1E0;
        
        if(reloclen)
        {
            *reloclen = (((uint8_t*)tptr - (uint8_t*)reloclen) + 3) & ~3;
            printf("Relocation %X\n", *reloclen);
        }
        
        printf("Bytes used: %X/%X, %u%%\n", currbyte, maxbyte, (currbyte * (100 * 0x10000) / maxbyte) >> 16);
    }
    while(0);
}

static uint32_t* fbTop = 0;
static uint32_t* fbBot = 0;

static uint32_t overlaytimer = ~0;
static uint32_t currentscale = 0;
static uint32_t redrawfb = 1;

static uint32_t* resosptr = 0;
static size_t resossize = 0;
static const uint16_t* textfb = 0;
static struct krn_kernel kernel;

static __attribute__((optimize("Ofast"))) void DoOverlay()
{
    uint32_t i, j, k = 0;
    
    if(!fbTop || !fbBot)
        return;
    
    if(overlaytimer && ~overlaytimer)
    {
        if(!--overlaytimer)
            redrawfb = 1;
    }
    
    if(resosptr && redrawfb)
    {
        if(kernel.pattern)
        {
            uint32_t* krnres;
            
            krnres = (uint32_t*)krn_calc(resosptr, 0, 256, 192, 0, 0, &kernel, &kernel, ~0);
            for(i = 0; i != 320; i++)
                for(j = 0; j != 240; j++)
                    putpixel(fbTop, krnres[(j << 9) + i], i + 40, j);
            
            krnres = (uint32_t*)krn_calc(resosptr + (256 * 192), 0, 256, 192, 0, 0, &kernel, &kernel, ~0);
            if(overlaytimer && textfb)
            {
                k = 0;
                for(i = 0; i != 320; i++)
                    for(j = 0; j != 240; j++)
                    {
                        //putpixel(fbBot, (krnres[(j << 9) + i] & 0xFCFCFCFC) >> 2, i, j);
                        
                        if(!textfb[k] || !i || i == 319)
                        {
                            fbBot[k++] = __builtin_bswap32((krnres[((239 - j) << 9) + i] & 0xFCFCFCFC) >> 2);
                            continue;
                        }
                        else
                        {
                            const uint32_t outlinepx = 0;
                            
                            if(!textfb[k - 240])
                            {
                                //fbTop[k - 240] = outlinepx;
                                fbBot[k - 240] = outlinepx;
                            }
                            if(!textfb[k - 1])
                            {
                                //fbTop[k - 1] = outlinepx;
                                fbBot[k - 1] = outlinepx;
                            }
                            if(!textfb[k + 1])
                            {
                                //fbTop[k + 1] = outlinepx;
                                fbBot[k + 1] = outlinepx;
                            }
                            if(!textfb[k + 240])
                            {
                                //fbTop[k + 240] = outlinepx;
                                fbBot[k + 240] = outlinepx;
                            }
                            
                            fbBot[k++] = ~0;
                            continue;
                        }
                    }
                return;
            }
            else
            {
                for(i = 0; i != 320; i++)
                    for(j = 0; j != 240; j++)
                        putpixel(fbBot, krnres[(j << 9) + i], i, j);
            }
        }
        else
        {
            for(j = 0; j != 192; j++)
                for(i = 0; i != 256; i++)
                    putpixel(fbTop, resosptr[k++], i + 72, j + 48);
            
            if(overlaytimer)
            {
                for(j = 0; j != 192; j++)
                    for(i = 0; i != 256; i++)
                        putpixel(fbBot, (resosptr[k++] & 0xFCFCFCFC) >> 2, i + 32, j);
            }
            else
            {
                for(j = 0; j != 192; j++)
                    for(i = 0; i != 256; i++)
                        putpixel(fbBot, resosptr[k++], i + 32, j);
            }
        }
    }
    
    if(!textfb)
        return;
    
    if(!redrawfb)
        return;
    
    if(!overlaytimer)
    {
        redrawfb = 0;
        return;
    }
    
    k = 241;
    do
    {
        if(!textfb[k])
            continue;
        else
        {
            const uint32_t outlinepx = 0;
            
            if(!textfb[k - 240])
            {
                //fbTop[k - 240] = outlinepx;
                fbBot[k - 240] = outlinepx;
            }
            if(!textfb[k - 1])
            {
                //fbTop[k - 1] = outlinepx;
                fbBot[k - 1] = outlinepx;
            }
            if(!textfb[k + 1])
            {
                //fbTop[k + 1] = outlinepx;
                fbBot[k + 1] = outlinepx;
            }
            if(!textfb[k + 240])
            {
                //fbTop[k + 240] = outlinepx;
                fbBot[k + 240] = outlinepx;
            }
            
            //                   RRRRRGGG GGGBBBBB
            //          BBBBBBBB GGGGGGGG RRRRRRRR
            // RRRRRRRR GGGGGGGG BBBBBBBB AAAAAAAA
            
            /*
            uint32_t px;
            
            px = textfb[k];
            px = (((px >>  8) & 0x0000F8) | ((px >> 13) & 0x000007))
               | (((px <<  5) & 0x00FC00) | ((px >>  1) & 0x000300))
               | (((px << 19) & 0xF80000) | ((px << 14) & 0x070000));
            //fbTop[k] = px;
            fbBot[k] = px;
            */
            fbBot[k] = ~0;
        }
    }
    while(++k != (240 * 319));
    
    redrawfb = 0;
}

static __attribute__((optimize("Ofast"))) void* memesearch(const void* patptr, const void* bitptr, const void* searchptr, size_t searchlen, size_t patsize)
{
    const uint8_t* pat = patptr;
    const uint8_t* bit = bitptr;
    const uint8_t* src = searchptr;
    
    size_t i = 0;
    size_t j = 0;
    
    searchlen -= patsize;
    
    if(bit)
    {
        while(i != searchlen)
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
    }
    else
    {
        while(i != searchlen)
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
    }
    
    return 0;
}

int main()
{
    gfxInit(GSP_RGBA8_OES, GSP_RGBA8_OES, false);
    
    gfxSetDoubleBuffering(GFX_BOTTOM, 0);
    
    osSetSpeedupEnable(1);
    
    PrintConsole menucon;
    consoleInit(GFX_TOP, &menucon);
    PrintConsole console;
    consoleInit(GFX_TOP, &console);
    
    puts("Hello, please wait while loading");
    
    textfb = malloc(400 * 240 * 2);
    menucon.frameBuffer = textfb;
    
    if(svcGetSystemTick() & 1)
        fwrite(dummy_bin, dummy_bin_size, 1, stdout);
    
    memset(&kernel, 0, sizeof(kernel));
    
    hidScanInput();
    agbg = (hidKeysHeld() & KEY_Y) ? 1 : 0;
    
    if(agbg)
        currentscale = 17;
    else
        currentscale = 7;
    
    u32 kDown = 0;
    u32 kHeld = 0;
    u32 kUp = 0;
    
    fbTop = (uint32_t*)gfxGetFramebuffer(GFX_TOP,    GFX_LEFT, 0, 0);
    fbBot = (uint32_t*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, 0, 0);
    
    uint8_t* codeptr = 0;
    size_t codesize = 0;
    uint32_t* codesizeptr = 0;
    
    uint8_t* codecptr = 0;
    size_t codecsize = 0;
    
    size_t mirfsize = 0;
    uint8_t* mirf = LoadSection0(&mirfsize);
    if(mirf)
    {
        puts("Processing, please wait...");
        size_t testaddr = 0;
        
        while(testaddr < mirfsize && *(const uint64_t*)(mirf + testaddr) != *(const uint64_t*)".code\0\0\0")
            testaddr += 0x200;
        if(testaddr < mirfsize)
        {
            codeptr = (uint8_t*)(mirf + testaddr + 0x200);
            codesizeptr = (uint32_t*)(mirf + testaddr + 12);
            codesize = lzss_dec(codeptr, 0, *codesizeptr);
            if(codesize)
            {
                codecptr = malloc(codesize);
                codecsize = lzss_dec(codeptr, codecptr, *codesizeptr);
                if(codesize == codecsize)
                {
                    const uint8_t* testresoos = (const uint8_t*)testimage_raw + testimage_raw_size - 8;
                    if(!*(--testresoos))
                    {
                        uint32_t hdr[2];
                        hdr[1]  = *(--testresoos) << 24;
                        hdr[1] |= *(--testresoos) << 16;
                        hdr[1] |= *(--testresoos) << 8;
                        hdr[1] |= *(--testresoos);
                        hdr[0]  = *(--testresoos) << 24;
                        hdr[0] |= *(--testresoos) << 16;
                        hdr[0] |= *(--testresoos) << 8;
                        hdr[0] |= *(--testresoos);
                        
                        
                        void* resostest = 0;
                        resossize = hdr[1] + (hdr[0] & 0xFFFFFF);
                        if(resossize == 0xC0000)
                        {
                            resosptr = malloc(resossize);
                            resostest = resosptr;
                            puts("Expanding stage1 resources");
                            size_t stage1size = lzss_dec(testimage_raw, 0, testimage_raw_size);
                            if(stage1size)
                            {
                                resossize = lzss_dec(testimage_raw, resosptr, testimage_raw_size);
                                if(resossize == stage1size)
                                {
                                    puts("Expanding stage2 resources");
                                    stage1size = lzss_dec(resosptr, 0, resossize);
                                    if(stage1size)
                                    {
                                        resossize = lzss_dec(resosptr, resosptr, resossize);
                                        if(resossize == 0xC0000)
                                        {
                                            DoOverlay();
                                            
                                            gfxFlushBuffers();
                                        }
                                        else resosptr = 0;
                                    }
                                    else resosptr = 0;
                                }
                                else resosptr = 0;
                            }
                            else resosptr = 0;
                        }
                        
                        if(!resosptr)
                            free(resostest);
                    }
                }
                else
                {
                    puts("Corrupted codebin");
                    mirf = 0;
                }
            }
            else
            {
                puts("Invalid codebin");
                mirf = 0;
            }
        }
        else
        {
            mirf = 0;
            puts("Can't find code in NCCH");
        }
    }
    else
    {
        puts("\n\nFailed to load TwlBg, please read above");
        puts("Push SELECT to exit");
    }
    
    
    if(mirf)
    {
        consoleSelect(&menucon);
        
        gfxSetScreenFormat(GFX_TOP, GSP_RGBA8_OES);
        gfxSetScreenFormat(GFX_BOTTOM, GSP_RGBA8_OES);
        
        gfxSwapBuffers();
    }
    
    fbTop = (uint32_t*)gfxGetFramebuffer(GFX_TOP,    GFX_LEFT, 0, 0);
    fbBot = (uint32_t*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, 0, 0);
    
    overlaytimer = 384;
    redrawfb = 1;
    
    krn_cvt(&kernel, scalelist1[currentscale].ptr, 5, 15);
    
    while(aptMainLoop())
    {
        hidScanInput();
        kDown = hidKeysDown();
        kHeld = hidKeysHeld();
        kUp = hidKeysUp();
        
        if(kHeld & KEY_SELECT)
            break;
        
        if(!mirf)
        {
            gspWaitForVBlank();
            continue;
        }
        
        if(((kHeld | kUp) & KEY_START) && !(kDown & KEY_START))
        {
            consoleSelect(&console);
            gfxSetScreenFormat(GFX_TOP, GSP_RGB565_OES);
            gfxSwapBuffers();
            gfxFlushBuffers();
            gspWaitForVBlank();
            
            puts("Doing patches");
            
            uint8_t* resptr = 0;
            
            if(currentscale && !agbg)
            {
                resptr = memesearch(scale1, 0, codecptr, codecsize, sizeof(scale1));
                if(resptr)
                {
                    //printf("memesearch %08X %08X %X\n", codecptr, resptr, resptr - codecptr);
                    puts("Doing kernel swap");
                    
                    memcpy(resptr, scalelist1[currentscale].ptr, sizeof(scale1));
                }
                else
                {
                    puts("Kernel swap failed :(");
                }
            }
            
            
            // === DMPGL patch for resolution changing
            
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
                    }
                }
            }
            
            // === unused MTX driver code
            
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
                while(it != 0x1E0);
                
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
                    uint32_t absaddr = (((resptr - codecptr) + 0x300000) & ~1) | 1;
                    *(res2ptr++) = absaddr;
                    *(res2ptr++) = absaddr >> 8;
                    *(res2ptr++) = absaddr >> 16;
                    *(res2ptr++) = absaddr >> 24;
                    
                    /*color_setting_t settest;
                    settest.temperature = 3200;
                    settest.gamma[0] = 1.0F;
                    settest.gamma[1] = 1.0F;
                    settest.gamma[2] = 1.0F;
                    settest.brightness = 1.0F;
                    calc_redshift(resptr, &settest);
                    */
                    calc_redshift(resptr, 0);
                    
                    
                    // === DS ONLY find relocation offset
                    if(!agbg)
                        resptr = memesearch(
                            (const uint8_t[]){0x7F, 0x00, 0x01, 0x00, 0x00, 0x00, 0xF0, 0x1E},
                            0,
                            codecptr, codecsize,
                            8
                        );
                    
                    if(!agbg && resptr)
                    {
                        // fileoffs + addroffs == vaddr
                        size_t addroffs = 0x400000 - *(u32*)(resptr + 8);
                        
                        // == find dummy error code return function called from MainLoop
                        res2ptr = memesearch(
                            (const uint8_t[]){0x00, 0x48, 0x70, 0x47, 0x00, 0x38, 0x40, 0xC9},
                            0,
                            codecptr, codecsize,
                            8
                        );
                        
                        // == find MainLoop function
                        resptr = memesearch(
                            (const uint8_t[]){0x80, 0x71, 0xDF, 0x0F, 0x18, 0x09, 0x12, 0x00},
                            (const uint8_t[]){0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00},
                            codecptr, codecsize,
                            8
                        );
                        
                        if(res2ptr && resptr)
                        {
                            resptr += 0x16; // the function is below the consts the memesearch is for
                            
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
                            
                            absaddr = 0x200000 | 1;
                            
                            *(res2ptr++) = absaddr;
                            *(res2ptr++) = absaddr >> 8;
                            *(res2ptr++) = absaddr >> 16;
                            *(res2ptr++) = absaddr >> 24;
                            
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
                    else if(!agbg)
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
            
            puts("Compressing... this will take a year or two");
            
            size_t outsize = ((*codesizeptr) + 0x1FF) & ~0x1FF;
            *codesizeptr = outsize;
            
            size_t lzres = (kHeld & KEY_R) ? ~0 : lzss_enc(codecptr, codeptr, codecsize, outsize);
            if(~lzres)
            {
                puts("Writing file to disk");
                mkdir("/luma", 0777);
                mkdir("/luma/sysmodules", 0777);
                char fnbuf[] = "/luma/sysmodules/TwlBg.cxi\0";
                if(agbg)
                    *(u32*)(fnbuf + 16) ^= 0xE101500;
                printf("fnbuf: %s\n", fnbuf);
                FILE* f = fopen(fnbuf, "wb");
                if(f)
                {
                    if(fwrite(mirf, mirfsize, 1, f) == 1)
                    {
                        fflush(f);
                        fclose(f);
                        puts("Patched TwlBg, ready to use!");
                    }
                    else
                    {
                        fclose(f);
                        puts("Failed to write TwlBg.cxi to disk :/");
                    }
                }
                else
                {
                    puts("Can't write TwlBg.cxi to disk...");
                    puts(" FAIL ");
                }
            }
            else
            {
                puts("Failed to compress...");
                puts("  Something went terribly wrong :/");
            }
            
            mirf = 0;
            
            puts("\n\nHold SELECT to exit");
            
            continue;
        }
        
        if(overlaytimer)
        {
            if(kDown & KEY_DOWN)
            {
                if(!scalelist1[++currentscale].ptr)
                {
                    while(scalelist1[--currentscale - 1].ptr)
                        /**/;
                }
                
                if(!agbg)
                    krn_cvt(&kernel, scalelist1[currentscale].ptr, 5, 15);
                else
                    krn_cvt(&kernel, scalelist1[currentscale].ptr, 6, 27);
            }
            
            if(kDown & KEY_UP)
            {
                if(!scalelist1[--currentscale].ptr)
                {
                    while(scalelist1[++currentscale + 1].ptr)
                        /**/;
                }
                
                if(!agbg)
                    krn_cvt(&kernel, scalelist1[currentscale].ptr, 5, 15);
                else
                    krn_cvt(&kernel, scalelist1[currentscale].ptr, 6, 27);
            }
        }
        
        if(kDown & KEY_X)
        {
            if(currentscale)
            {
                if(!agbg)
                    krn_cvt(&kernel, scale1, 5, 15);
                else
                    krn_cvt(&kernel, scale2, 6, 27);
            }
            else
                kernel.pattern = 0;
        }
        
        if(kUp & KEY_X)
        {
            if(!agbg)
                krn_cvt(&kernel, scalelist1[currentscale].ptr, 5, 15);
            else
                krn_cvt(&kernel, scalelist1[currentscale].ptr, 6, 27);
        }
        
        if((kHeld & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_START))
        || ((kUp | kDown) & (KEY_A | KEY_B))
        )
            overlaytimer = 96;
        
        if(kDown | (kUp & KEY_X))
            redrawfb = 1;
        
        // start print menu
        
        menucon.cursorX = 0;
        menucon.cursorY = 1;
        
        puts("TWPatch by Sono (build " DATETIME ")\n");
        
        
        puts("Scale list:");
        
        do
        {
            size_t iter = 1;
            
            if(agbg)
            {
                while(scalelist1[iter++].ptr)
                    /**/;
            }
            
            do
            {
                printf("%c - %s\n", iter == currentscale ? '>' : ' ', scalelist1[iter].name);
                ++iter;
            }
            while(scalelist1[iter].name || scalelist1[iter].ptr);
        }
        while(0);
        
        puts("\nHold X for one second to switch between");
        puts("  Nintendo scaling and current scaling");
        puts("\n  The UI is very slow, so hold the");
        puts("    button for at least 1 second!");
        puts("\n\n Hold START to save and exit");
        puts(" Hold SELECT to just exit");
        
        if(kDown & KEY_START)
        {
            menucon.cursorX = 0;
            menucon.cursorY = 1;
            
            size_t iter = 28;
            
            do
                puts("Compression takes forever\e[K");
            while(--iter);
        }
        
        // end print menu
        
        DoOverlay();
        
        gspWaitForVBlank();
    }
    
    //ded:
    
    gfxExit();
    
    return 0;
}
