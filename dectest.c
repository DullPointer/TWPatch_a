#include <stdint.h>

void ct_dec(const uint32_t const* compress_t, volatile uint32_t* addr)
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t prev = 0;
    uint32_t compress_o = 0;
    
    addr[0x00] = 0;
    addr[0x40] = 0;
    
    for(i = 0; i < 0x100; i++)
    {
        for(j = 0; j < 4; j++)
        {
            while(compress_t[compress_o >> 5] & (1 << (compress_o & 31)))
            {
                compress_o++;
                prev += 1 << (8 * j);
            }
            
            compress_o++;
        }
        
        addr[0x01] = prev;
        addr[0x41] = prev;
    }
}
