#include <3ds.h>
#include <stdlib.h>
#include <string.h>

#include "krn.h"

static inline int count_bits(int value, int mask)
{
    int ret = 0;
    while(value)
    {
        if(mask || (value & 1))
            ret++;
        value >>= 1;
    }
    return ret;
}

void  __attribute__((optimize("Ofast"))) krn_cvt(struct krn_kernel* krn, const s16* krndata, u32 krnx, u32 krnbits)
{
    memset(krn->matrix, 0, sizeof(krn->matrix));
    
    u32 i, j;
    
    for(j = 0; j != 6; j++)
        for(i = 0; i != krnx; i++)
            krn->matrix[i][j] = (int)krndata[(krnx*j)+i];
    
    krn->width = krnx;
    krn->pattern = krnbits;
}

static inline __attribute__((optimize("Ofast"))) u32 do_fifo_mult(const u32* fifo, const int* mtx)
{
    u32 i, j, ret = 0;
    
    for(i = 0; i != 4; i++)
    {
        int color = 0;
        for(j = 0; j != 6; j++)
            color += (int)((fifo[j] >> (i << 3)) & 0xFF) * mtx[j];
        if(color > 0x3FFFFF)
            color = 0x3FFFFF;
        else if(color < 0)
            color = 0;
        
        ret = (ret >> 8) | ((color << 10) & 0xFF000000);
    }
    
    return ret;
}

static __attribute__((optimize("Ofast"))) void do_matrix_line(const u32* bufin, u32* bufout, u32 size, const struct krn_kernel* krn)
{
    u32 offs = 2;
    u32 fifo[6];
    
    fifo[3] = *bufin;
    fifo[2] = fifo[3];
    fifo[1] = fifo[2];
    fifo[0] = fifo[1];
    fifo[4] = *(++bufin);
    fifo[5] = *(++bufin);
    
    u32 localpat, rem, mtxi;
    
    do
    {
        localpat = krn->pattern;
        rem = krn->width - 1;
        mtxi = 0;
        
        do
        {
            *(bufout++) = do_fifo_mult(fifo, krn->matrix[mtxi++]);
            ++offs;
            
            if(localpat & 1)
            {
                fifo[0] = fifo[1];
                fifo[1] = fifo[2];
                fifo[2] = fifo[3];
                fifo[3] = fifo[4];
                fifo[4] = fifo[5];
                if(offs < size)
                    fifo[5] = *(++bufin);
            }
            localpat >>= 1;
        }
        while(rem--);
    }
    while(offs < size);
}

static __attribute__((optimize("Os"))) void do_matrix_raw(const u32* bufin, u32* bufout, u32 width, u32 height, u32 origwidth, u32 origheight,
    const struct krn_kernel* kernelx, const struct krn_kernel* kernely)
{
    u32 templine1[512];
    u32 templine2[512];
    u32 i, j;
    for(i = 0; i != origheight; i++)
    {
        //printf("do_matrix_line x %i\n", i);
        do_matrix_line(bufin + (i * origwidth), bufout + (i << 9), width - 1, kernelx);
    }
    for(i = 0; i != width; i++)
    {
        //printf("do_matrix_line y %i\n", i);
        
        for(j = 0; j != height; j++)
            templine1[j] = bufout[(j << 9) | i];
        do_matrix_line(templine1, templine2, height - 1, kernely);
        for(j = 0; j != height; j++)
            bufout[(j << 9) | i] = templine2[j];
    }
}

static u32* hwbuf = 0;

size_t __attribute__((optimize("Os"))) krn_calc(const void* bufin, void* bufout,
    u32 inwidth, u32 inheight, u32* outwidth, u32* outheight,
    const struct krn_kernel* krnx, const struct krn_kernel* krny, u32 outstride)
{
    int onbitsx = count_bits(krnx->pattern, 0); //input count
    int onbitsy = count_bits(krny->pattern, 0); //input count
    
    u32 calcwidth = inwidth * krnx->width / onbitsx;
    u32 calcheight = inheight * krny->width / onbitsy;
    
    //printf("upscale %i/%i %ix%i --> %ix%i\n", krn->width, onbits, inwidth, inheight, calcwidth, calcheight);
    
    if(outwidth)
        *outwidth = calcwidth;
    if(outheight)
        *outheight = calcheight;
    
    if(!bufout && !outstride) //size check only
        return 1;
    
    if(!hwbuf)
        hwbuf = malloc(512 * 512 * 4);
    do_matrix_raw(bufin, hwbuf, calcwidth, calcheight, inwidth, inheight, krnx, krny);
    
    if(!bufout)
        return (size_t)hwbuf;
    
    u32 j;
    for(j = 0; j != calcheight; j++)
    {
        memcpy(bufout, &hwbuf[j << 9], calcwidth * 4);
        bufout += outstride;
    }
    
    return 1;
}
