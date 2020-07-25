#ifndef TW_COMMON_MAIN
#define TW_COMMON_MAIN

int agbg = 0;

static void* LoadSection0(size_t* outsize)
{
#ifdef _3DS
    // path to /title/00040138/?0000?02/*.app!exefs.bin
    u32 apath[4];
    apath[0] = 0x20000002 | (1 << (8 | agbg));
    apath[1] = 0x00040138;
    apath[2] = 0;
    apath[3] = 0;
    
    // path to sub-file in exefs.bin
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
#endif
    
    size_t codesize;
    size_t offs = 0;
    void* ret = 0;
    uint32_t rd = 0;
    
#ifdef _3DS
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
#endif
        
        uint8_t buf[0x200];
        //printf("OpenFileDirectly fail %08X\n", res);
        //puts("Can't open TWL_FIRM from NAND");
#ifdef _3DS
        FILE* f = fopen(!agbg ? "/luma/section0.bin" : "/luma/agbg0.bin", "rb");
#else
        FILE* f = fopen(!agbg ? "section0.bin" : "agbg0.bin", "rb");
#endif
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
        
        // ^D code slide
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
        
        // pNCCH->SizeInBlocks * 512 // 512 is block size
        codesize = *(const uint32_t*)(buf + 0x104) << 9;
        ret = malloc((codesize + 0x1FF) & ~0x1FF);
        if(!ret)
        {
            printf("Corrupt NCCH size %X\n", *(const uint32_t*)(buf + 0x104));
            fclose(f);
            return 0;
        }
        
        memcpy(ret, buf, 0x200);
        
        // name always seems to be located at pNCCH + 1
        //if(fread(buf, 0x200, 1, f) != 1 || *(const uint64_t*)buf != *(const uint64_t*)"TwlBg\0\0")
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
#ifdef _3DS
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
    
    // EOR testing for both TwlBg and AgbBg
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
#endif
}

uint8_t* ParseMIRF(uint8_t* mirf, size_t mirfsize, size_t* decodecsize, uint32_t** decodesizeptr, uint8_t** decodeptr)
{
    size_t testaddr = 0;
    
    while(testaddr < mirfsize && *(const uint64_t*)(mirf + testaddr) != *(const uint64_t*)".code\0\0\0")
        testaddr += 0x200;
    if(testaddr < mirfsize)
    {
        uint8_t* codeptr = (uint8_t*)(mirf + testaddr + 0x200);
        uint32_t* codesizeptr = (uint32_t*)(mirf + testaddr + 12);
        size_t codesize = lzss_dec(codeptr, 0, *codesizeptr);
        if(codesize)
        {
            uint8_t* codecptr = malloc(codesize);
            size_t codecsize = lzss_dec(codeptr, codecptr, *codesizeptr);
            if(codesize != codecsize)
            {
                free(codecptr);
                puts("Corrupted codebin");
                return 0;
            }
            
            if(decodecsize)
                *decodecsize = codecsize;
            if(decodesizeptr)
                *decodesizeptr = codesizeptr;
            if(decodeptr)
                *decodeptr = codeptr;
            return codecptr;
        }
        else
        {
            puts("Invalid codebin");
            return 0;
        }
    }
    else
    {
        puts("Can't find code in NCCH");
        return 0;
    }
}

#else
#error Only include this file in the main compile unit!
#endif