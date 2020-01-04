#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

__attribute__((optimize("Ofast"))) static const void* memesearch(const void* patptr, const void* bitptr, const void* searchptr, size_t searchlen, size_t patsize)
{
    if(searchlen < patsize)
        return 0;
    
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
                
                /*printf("memesearch bit %X %X\n", i, j);
                for(j = 0; j != patsize; j++) printf("%02X", src[i + j]);
                puts("");
                for(j = 0; j != patsize; j++) printf("%02X", pat[j]);
                puts("");
                for(j = 0; j != patsize; j++) printf("%02X", bit[j]);
                puts("");*/
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
                
                /*printf("memesearch nrm %X %X\n", i, j);
                for(j = 0; j != patsize; j++) printf("%02X", src[i + j]);
                puts("");
                for(j = 0; j != patsize; j++) printf("%02X", pat[j]);
                puts("");*/
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

static uint8_t* readfile(const char* path, size_t* filesize)
{
    FILE* f = fopen(path, "rb");
    if(!f)
        return 0;
    
    fseek(f, 0, SEEK_END);
    
    size_t size = ftell(f);
    
    fseek(f, 0, SEEK_SET);
    
    uint8_t* data = malloc(size);
    if(!data)
    {
        fclose(f);
        return 0;
    }
    
    if(fread(data, size, 1, f) != 1)
    {
        free(data);
        fclose(f);
        
        return 0;
    }
    
    fclose(f);
    
    if(filesize)
        *filesize = size;
    
    return data;
}

static const char* num = "\0\x01\x02\x03\x04\x05\x06\x07\x08\x09\0\0\0\0\0\0\0\x0A\x0B\x0C\x0D\x0E\x0F\0\0\0\0\0\0\0\0\0\0";

static size_t tohex(char* ptr)
{
    char* d = ptr;
    
    size_t i = 0;
    
    for(;;)
    {
        char c1 = *(ptr++);
        char c2 = *(ptr++);
        
        if(!c1) return i;
        if(!c2) return 0;
        
        c1 = num[((c1 & 0x1F) ^ 0x10)] << 4;
        c2 = num[((c2 & 0x1F) ^ 0x10)];
        
        *(d++) = c1 | c2;
        
        i++;
        
        for(;;)
        {
            char cn = *ptr;
            
            if
            (
                   cn != ' '
                && cn != '-'
                && cn != '_'
                && cn != ';'
                && cn != ','
                && cn != '.'
                && cn != ':'
            )
                break;
            else
                ptr++;
        }
    }
}

int main(int argc, char** argv)
{
    if(argc < 3 || argc > 4)
        return 1;
    
    uint8_t* pat = argv[2];
    uint8_t* mask = 0;
    
    size_t patlen = tohex(argv[2]);
    size_t masklen = patlen;
    
    if(!patlen)
    {
        puts("Invalid data hex");
        return 1;
    }
    
    if(argc == 4)
    {
        masklen = tohex(argv[3]);
        
        if(!masklen)
        {
            puts("Invalid mask hex");
            return 1;
        }
        
        if(patlen != masklen)
        {
            puts("Mask-data len mismatch");
            return 1;
        }
        
        mask = argv[3];
    }
    
    size_t datalen = 0;
    
    uint8_t* data = readfile(argv[1], &datalen);
    const uint8_t* dataend = data + datalen;
    
    if(!data)
    {
        printf("Failed to read file '%s': (%i) %s\n", argv[1], errno, strerror(errno));
        
        return 1;
    }
    
    const uint8_t* ptr = data;
    
    for(;;)
    {
        ptr = memesearch(pat, mask, ptr, dataend - ptr, patlen);
        if(!ptr)
            break;
        
        printf("%08X:", ptr - data);
        
        int i;
        
        for(i = 0; i != patlen; i++)
            printf(" %02X", ptr[i]);
        
        putchar('\n');
        
        ptr += 1;
    }
    
    free(data);
    
    return 0;
}
