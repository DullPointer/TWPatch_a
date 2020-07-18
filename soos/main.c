/*
    LgyPatPat - TwlBg Matrix hardware matrix patcher
    Copyright (C) 2019-2020 Sono (https://github.com/SonoSooS)
    All Rights Reserved
*/

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "dummy_bin.h"
#include "testimage_raw.h"

#include "lz.h"
#include "krn.h"
#include "red.h"
#include "pat.h"

#include "common_main.h"

#include "krnlist_all.h"

// I don't know of a faster way, but this is also used for rotating
static inline void putpixel(uint32_t* fb, uint32_t px, uint32_t x, uint32_t y)
{
    fb[(y * 240) + x] = px;
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

static uint32_t currscreen = 0;

static color_setting_t redset;
static int curred = 0;

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
            
            krnres = (uint32_t*)krn_calc(resosptr, 0, 192, 256, 0, 0, &kernel, &kernel, ~0);
            memcpy(fbTop + (40 * 240), krnres, 240 * 320 * 4);
            
            krnres = (uint32_t*)krn_calc(resosptr + (192 * 256), 0, 192, 256, 0, 0, &kernel, &kernel, ~0);
            if(overlaytimer && textfb)
            {
                for(k = 0; k != (240 * 320); k++)
                {
                    if(textfb[k] && k >= 240)
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
                        
                        fbBot[k] = ~0;
                        continue;
                    }
                    else
                    {
                        fbBot[k] = (krnres[k] & 0xFCFCFCFC) >> 2;
                        continue;
                    }
                }
                return;
            }
            else
            {
                memcpy(fbBot, krnres, 240 * 320 * 4);
            }
        }
        else
        {
            for(i = 0; i != 256; i++, k+= 192)
                memcpy(fbTop + (240 * (72 + i)), resosptr + k, 192 * 4);
            
            if(overlaytimer)
            {
                for(j = 0; j != 256; j++)
                    for(i = 0; i != 192; i++)
                        putpixel(fbBot, (resosptr[k++] & 0xFCFCFCFC) >> 2, i + 48, j + 72);
            }
            else
            {
                for(i = 0; i != 256; i++, k+= 192)
                    memcpy(fbBot + 48 + (240 * i), resosptr + k, 192 * 4);
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

static void WriteAll(const u32* dst, int screen)
{
    u32 reg = screen & 1 ? 0x400580 : 0x400480;
    
    u8 idx = 0;
    do
    {
        u32 pos = idx;
        GSPGPU_WriteHWRegs(reg + 0, &pos, 4);
        GSPGPU_WriteHWRegs(reg + 4, dst++, 4);
    }
    while(++idx);
}

static void ApplyCS(color_setting_t* cs, int screen)
{
    u8 i = 0;
    
    struct
    {
        u8 r;
        u8 g;
        u8 b;
        u8 z;
    } px[256];
    
    do
    {
        *(u32*)&px[i] = i | (i << 8) | (i << 16);
    }
    while(++i);
    
    if(cs)
    {
        u16 c[256 * 3];
        do
        {
            *(c + i + 0x000) = px[i].r | (px[i].r << 8);
            *(c + i + 0x100) = px[i].g | (px[i].g << 8);
            *(c + i + 0x200) = px[i].b | (px[i].b << 8);
        }
        while(++i);
        
        colorramp_fill(c + 0x000, c + 0x100, c + 0x200, 256, cs);
        
        do
        {
            px[i].r = *(c + i + 0x000) >> 8;
            px[i].g = *(c + i + 0x100) >> 8;
            px[i].b = *(c + i + 0x200) >> 8;
        }
        while(++i);
    }
    
    if(screen & 1)
        WriteAll((u32*)px, 0);
    if(screen & 2)
        WriteAll((u32*)px, 1);
}

static struct
{
    size_t mask;
    const char* label;
} patlist[] =
{
    {PAT_REDSHIFT, "Redshift"},
    {PAT_WIDE, "DMPGL Wide test 384x240 16:10"},
    {PAT_GPUSCALING, "GPU scale test (health hazard!)"},
    {PAT_EHANDLER, "Install exception handler"},
    {PAT_HID, "Anti-DPAD"},
    {PAT_HOLE, "Hole (obsolete)"},
    {PAT_DEBUG, "???"},
    //{PAT_HOLE | PAT_HID | PAT_GPUSCALING | (1 << 30), "Ktest"},
    {1 << 30, "* No default patches"},
    {0, 0},
    {PAT_REDSHIFT, "Redshift"},
    {PAT_WIDE, "DMPGL Wide test 400x240 15:9"},
    {PAT_GPUSCALING, "GPU scale test (health hazard!)"},
    {PAT_EHANDLER, "Install exception handler"},
    {PAT_HID, "Anti-DPAD"},
    {PAT_HOLE, "Hole (obsolete)"},
    {PAT_DEBUG, "???"},
    //{PAT_GPUSCALING | PAT_EHANDLER | PAT_HID | (1 << 30), "BetterAGB debug"},
    {1 << 30, "* No default patches"},
    {0, 0}
};

static size_t currpat = 0;
static size_t patmask = 0;

size_t lzss_enc2(const void* __restrict in, void* __restrict out, size_t insize, size_t outsize);

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
    
    textfb = malloc(240 * 400 * 2);
    menucon.frameBuffer = textfb;
    
    if(svcGetSystemTick() & 1)
        fwrite(dummy_bin, dummy_bin_size, 1, stderr);
    
    memset(&kernel, 0, sizeof(kernel));
    
    hidScanInput();
    agbg = (hidKeysHeld() & KEY_Y) ? 1 : 0;
    
    currentscale = 8;
    
    if(agbg)
    {
        while(scalelist1[currentscale++].ptr)
            ;
        
        currentscale += 0;
        
        while(patlist[currpat++].label)
            ;
        
        currpat += 0;
    }
    
    u32 kDown = 0;
    u32 kHeld = 0;
    u32 kUp = 0;
    
    fbTop = (uint32_t*)gfxGetFramebuffer(GFX_TOP,    GFX_LEFT, 0, 0);
    fbBot = (uint32_t*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, 0, 0);
    
    uint8_t* codeptr = 0;
    uint32_t* codesizeptr = 0;
    
    uint8_t* codecptr = 0;
    size_t codecsize = 0;
    
    size_t mirfsize = 0;
    uint8_t* mirf = LoadSection0(&mirfsize);
    if(mirf)
    {
        puts("Processing, please wait...");
        
        codecptr = ParseMIRF(mirf, mirfsize, &codecsize, &codesizeptr, &codeptr);
        if(codecptr)
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
    
    memset(&redset, 0, sizeof(redset));
    redset.temperature = 3200;
    redset.gamma[0] = 1.0F;
    redset.gamma[1] = 1.0F;
    redset.gamma[2] = 1.0F;
    redset.brightness = 1.0F;
    
    color_setting_t colorset;
    memcpy(&colorset, &redset, sizeof(colorset));
    
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
        
        if(!currscreen)
        {
            if(((kHeld | kUp) & KEY_START) && !(kDown & KEY_START))
            {
                consoleSelect(&console);
                gfxSetScreenFormat(GFX_TOP, GSP_RGB565_OES);
                gfxSwapBuffers();
                gfxFlushBuffers();
                gspWaitForVBlank();
                
                size_t mask = (patmask & (1 << 30)) ? (patmask & 0xFFFF) : (PAT_HID | PAT_RTCOM | patmask);
                if(kHeld & KEY_X)
                {
                    puts("Widescreen patches enabled!\n");
                    mask |= PAT_WIDE;
                }
                if(kHeld & KEY_Y)
                {
                    puts("rtcom is disabled, which can");
                    puts(" lead to instability\n");
                    mask &= ~PAT_RTCOM;
                }
                
                puts("\nDoing patches\n");
                
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
                
                pat_apply(codecptr, codecsize, &redset, mask);
                
                puts("Compressing... this will take a year or two");
                
                size_t outsize = ((*codesizeptr) + 0x1FF) & ~0x1FF;
                *codesizeptr = outsize;
                
                u64 start = svcGetSystemTick();
                
                size_t lzres = (kHeld & KEY_R) ? ~0 : lzss_enc2(codecptr, codeptr, codecsize, outsize);
                if(~lzres)
                {
                    start = ((svcGetSystemTick() - start) >> 4) / 16756991;
                    printf("Took %llum%llus\n", start / 60, start % 60);
                    
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
            
            if((kHeld & KEY_Y) && (kDown & (KEY_Y | KEY_B)) == KEY_B)
            {
                currscreen = 2;
                consoleClear();
            }
        }
        else if(currscreen == 1)
        {
            if(kDown & KEY_X)
            {
                memset(&redset, 0, sizeof(redset));
                redset.temperature = 6500;
                redset.gamma[0] = 1.0F;
                redset.gamma[1] = 1.0F;
                redset.gamma[2] = 1.0F;
                redset.brightness = 1.0F;
                
                ApplyCS(0, 3);
            }
            
            if((((kDown & KEY_RIGHT) ? 1 : 0) ^ ((kDown & KEY_LEFT) ? 1 : 0)))
            {
                if(!curred)
                {
                    if(kDown & KEY_RIGHT)
                        redset.temperature += (kHeld & (KEY_L | KEY_R)) ? 1 : 100;
                    else
                        redset.temperature -= (kHeld & (KEY_L | KEY_R)) ? 1 : 100;
                }
                else
                {
                    float* f = 0;
                    if(!(curred >> 2))
                        f = &redset.gamma[curred - 1];
                    
                    if(!(curred ^ 4))
                        f = &redset.brightness;
                    
                    if(f)
                    {
                        if(kDown & KEY_RIGHT)
                            *f += (kHeld & (KEY_L | KEY_R)) ? 0.01F : 0.1F;
                        else
                            *f -= (kHeld & (KEY_L | KEY_R)) ? 0.01F : 0.1F;
                    }
                }
                
                if(redset.temperature < 1000)
                    redset.temperature = 1000;
                if(redset.temperature > 25100)
                    redset.temperature = 25100;
                
                if(redset.gamma[0] < 0.01F)
                    redset.gamma[0] = 0.01F;
                if(redset.gamma[1] < 0.01F)
                    redset.gamma[1] = 0.01F;
                if(redset.gamma[2] < 0.01F)
                    redset.gamma[2] = 0.01F;
                
                if(redset.gamma[0] > 256.0F)
                    redset.gamma[0] = 256.0F;
                if(redset.gamma[1] > 256.0F)
                    redset.gamma[1] = 256.0F;
                if(redset.gamma[2] > 256.0F)
                    redset.gamma[2] = 256.0F;
                
                if(redset.brightness < 0.01F) redset.brightness = 0.01F;
                if(redset.brightness > 1.0F) redset.brightness = 1.0F;
                
                ApplyCS(&redset, 3);
            }
            
            if(kDown & KEY_DOWN)
            {
                if(++curred > 4)
                    curred = 4;
            }
            
            if(kDown & KEY_UP)
            {
                if(--curred < 0)
                    curred = 0;
            }
            
            if(kDown & KEY_B)
            {
                currscreen = 0;
                consoleClear();
                
                ApplyCS(0, 3);
            }
        }
        else if(currscreen == 2)
        {
            if(kDown & KEY_A)
            {
                if
                (
                    ((patmask & (1 << 30)) || !(patlist[currpat].mask & (1 << 31)))
                    &&
                    (patlist[currpat].mask & patmask)
                )
                    patmask &= ~patlist[currpat].mask;
                else
                    patmask |= patlist[currpat].mask & ~(1 << 31);
            }
            
            if(kDown & KEY_DOWN)
            {
                if(!patlist[++currpat].label)
                {
                    while(patlist[--currpat - 1].label)
                        /**/;
                }
            }
            
            if(kDown & KEY_UP)
            {
                if(!patlist[--currpat].label)
                {
                    while(patlist[++currpat + 1].label)
                        /**/;
                }
            }
            
            if(kDown & KEY_B)
            {
                if(kHeld & KEY_Y)
                {
                    currscreen = 1;
                    ApplyCS(&redset, 3);
                }
                else
                    currscreen = 0;
                
                consoleClear();
            }
        }
        else
        {
            currscreen = 0;
            consoleClear();
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
        
        if(currscreen || (kHeld & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_START))
        || ((kUp | kDown) & (KEY_A | KEY_B))
        )
            overlaytimer = 96;
        
        if(kDown | (kUp & KEY_X))
            redrawfb = 1;
        
        // start print menu
        
        menucon.cursorX = 0;
        menucon.cursorY = 1;
        
        puts("TWPatch by Sono (build " DATETIME ")\n");
        
        if(!currscreen)
        {
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
            
            puts("\n Hold X to see the original pattern");
            
            //puts("\n  The UI is very slow, so hold the");
            //puts("    button for at least 1 second!");
            
            puts("\n Hold Y + B to access the patch menu");
            
            puts("\n Hold START to save and exit");
            puts("      + X to enable widescreen");
            puts("      + Y to disable rtcom");
            
            puts("\n Hold SELECT to exit");
            
            if(kDown & KEY_START)
            {
                menucon.cursorX = 0;
                menucon.cursorY = 1;
                
                size_t iter = 28;
                
                do
                    puts("Compression takes forever\e[K");
                while(--iter);
            }
        }
        else if(currscreen == 1)
        {
            puts("CTRTWL_Redshift v0.0 control panel");
            
            printf("\n%c Colortemp: %iK\n\n", (curred == 0 ? '>' : ' '), redset.temperature);
            
            printf("%c Gamma[R]: %.2f\n", (curred == 1 ? '>' : ' '), redset.gamma[0]);
            printf("%c Gamma[G]: %.2f\n", (curred == 2 ? '>' : ' '), redset.gamma[1]);
            printf("%c Gamma[B]: %.2f\n", (curred == 3 ? '>' : ' '), redset.gamma[2]);
            
            printf("\n%c Brightness: %.2f\n\n", (curred == 4 ? '>' : ' '), redset.brightness);
        }
        else if(currscreen == 2)
        {
            puts("Patches\n");
            
            do
            {
                size_t iter = 0;
                
                if(agbg)
                {
                    while(patlist[iter++].label)
                        /**/;
                }
                
                do
                {
                    printf("%c - %c %s\n",
                        iter == currpat ? '>' : ' ',
                        (!(patmask & (1 << 30)) && (patlist[iter].mask & (1 << 31)))
                        ||
                        (patlist[iter].mask & patmask) == (patlist[iter].mask & ~(1 << 31))
                        ? 'x'
                        : !((patlist[iter].mask & patmask) & ~(1 << 31)) ? '.' : '?',
                        patlist[iter].label);
                    ++iter;
                }
                while(patlist[iter].label);
            }
            while(0);
            
            puts("\n Press A to toggle selected patch");
            puts("\n Press B to go back");
            if(patmask & PAT_REDSHIFT)
                puts("\n Hold Y + B to open CTR_Redshift configurator");
        }
        else
        {
            puts("lol");
        }
        
        // end print menu
        
        DoOverlay();
        
        gspWaitForVBlank();
    }
    
    //ded:
    
    gfxExit();
    
    return 0;
}
