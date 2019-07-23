#include <stdint.h>

uint8_t font[8 * 16];

void font_draw(uint32_t* fb, uint32_t val, uint32_t y, uint32_t x)
{
    int i = 32 - 4;
    
    fb += (y * 180 * 8);
    fb += x * 8;
    
    for(;;)
    {
        int sw = (val >> i) & 0xF;
        
        const uint8_t* fnptr = font + (sw * 8);
        
        for(y = 0; y != 8; y++)
        {
            uint8_t fnt = fnptr[y];
            
            for(x = 0; x != 8; x++)
            {
                if(fnt & (1 << x))
                    fb[x] = ~0;
                else
                    fb[x] = 0;
            }
            
            fb += 180;
        }
        
        //fb -= 8 * 180;
        //fb += 8;
        
        if(!i)
            break;
        i -= 4;
    }
}
