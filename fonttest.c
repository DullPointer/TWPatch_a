#include <stdint.h>

void font_draw(uint8_t* fb, uint32_t val)
{
    int i = 32 - 4;
    
    uint32_t x, y, z;
    
    for(;;)
    {
        uint32_t sw = (val >> i) & 0xF;
        
        for(y = 0; y != 8; y++)
        {
            uint32_t mask = 1 << (5 + ((sw >> 3) << 1));
            uint32_t color = 0;
            if(sw & 8) color |= mask | (mask << 8) | (mask << 16);
            
            if(sw & 4) color |= 0x80;
            color <<= 8;
            if(sw & 2) color |= 0x80;
            color <<= 8;
            if(sw & 1) color |= 0x80;
            
            for(x = 0; x != 24;)
                for(z = 0; z != 24; z += 8)
                    fb[x++] = color >> z;
            
            fb += 240 * 3;
        }
        
        //fb -= 8 * 180;
        //fb += 8;
        
        if(!i)
            break;
        i -= 4;
    }
}
