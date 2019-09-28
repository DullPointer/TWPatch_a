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
            uint32_t v = sw >> 3;
            v |= v << 8;
            v |= v << 16;
            v |= ((sw     ) & 1) << 17;
            v |= ((sw >> 1) & 1) << 9;
            v |= ((sw >> 2) & 1) << 1;
            v |= v << 2;
            v |= v << 4;
            
            for(x = 0; x != 24;)
                for(z = 0; z != 24; z += 8)
                    fb[x++] = v >> z;
            
            fb += 240 * 3;
        }
        
        //fb -= 8 * 180;
        //fb += 8;
        
        if(!i)
            break;
        i -= 4;
    }
}
