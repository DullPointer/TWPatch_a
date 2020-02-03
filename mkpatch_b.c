#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "soos/lz.h"
#include "soos/red.h"
#include "soos/pat.h"

#include "mkpatch_a.h"

#include "soos/common_main.h"

int main(int argc, char** argv)
{
    size_t pat = PAT_HID | PAT_RTCOM;
    
    if(argv[1])
    {
        if(!strcmp("agb", argv[1]))
        {
            puts("AGBG Debug");
            agbg = 1;
        }
        
        if(argv[2])
        {
            const char* aaa = argv[2];
            
            pat = 0;
            
            while(*aaa)
            {
                pat = (pat << 1) | (*aaa & 1);
                ++aaa;
            }
        }
    }
    
    if(pat & PAT_RELOC)
    {
        puts("Relocation patch is obsolete! Please unmask the bit!");
        return 1;
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
        codecptr = ParseMIRF(mirf, mirfsize, &codecsize, &codesizeptr, &codeptr);
        if(!codecptr)
        {
            puts("Corrupted codebin");
            return 1;
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
    pat_apply(codecptr, codecsize, &settest, pat);
    
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
