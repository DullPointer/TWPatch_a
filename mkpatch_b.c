#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "soos/lz.h"
#include "soos/red.h"
#include "soos/pat.h"

#include "mkpatch_a.h"

int agbg = 0;

static void* LoadSection0(size_t* outsize)
{
    size_t codesize;
    void* ret = 0;
    
    uint8_t buf[0x200];
    //printf("OpenFileDirectly fail %08X\n", res);
    //puts("Can't open TWL_FIRM from NAND");
    FILE* f = fopen(!agbg ? "section0.bin" : "agbg0.bin", "rb");
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
    
    if(fread(buf, 0x200, 1, f) != 1 || ((*(const uint64_t*)buf ^ *(const uint64_t*)"TwlBg\0\0") & ~0xE1015))
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

int main(int argc, char** argv)
{
    if(argv[1] && !strcmp("agb", argv[1]))
    {
        puts("AGBG Debug");
        agbg = 1;
    }
    
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
                if(codesize != codecsize)
                {
                    puts("Corrupted codebin");
                    free(mirf);
                    mirf = 0;
                }
            }
            else
            {
                puts("Invalid codebin");
                free(mirf);
                mirf = 0;
            }
        }
        else
        {
            free(mirf);
            mirf = 0;
            puts("Can't find code in NCCH");
        }
    }
    else
    {
        puts("\n\nFailed to load TwlBg, please read above");
        puts("Push SELECT to exit");
        
        return 1;
    }
    
    puts("Doing patches");
    
    uint8_t* resptr = 0;
    
    color_setting_t settest;
    settest.temperature = 3200;
    settest.gamma[0] = 1.0F;
    settest.gamma[1] = 1.0F;
    settest.gamma[2] = 1.0F;
    settest.brightness = 1.0F;
    pat_apply(codecptr, codecsize, &settest, ~0 & ~PAT_REDSHIFT);
    
    puts("Compressing... this will take a year or two");
    
    size_t outsize = ((*codesizeptr) + 0x1FF) & ~0x1FF;
    *codesizeptr = outsize;
    
    size_t lzres = lzss_enc(codecptr, codeptr, codecsize, outsize);
    if(~lzres)
    {
        puts("Writing file to disk");
        char fnbuf[] = "/TwlBg.cxi\0";
        if(agbg)
            *(uint32_t*)(fnbuf) ^= 0xE101500;
        printf("fnbuf: %s\n", fnbuf);
        FILE* f = fopen(fnbuf + 1, "wb");
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
    
    return 0;
}
