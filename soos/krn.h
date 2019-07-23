#pragma once

struct krn_kernel
{
    u32 width;
    u32 pattern;
    int matrix[8][6]; //on real hardware it's [6][8]
};

void krn_cvt(struct krn_kernel* krn, const s16* krndata, u32 krnx, u32 krnbits);
size_t krn_calc(const void* bufin, void* bufout, u32 inwidth, u32 inheight, u32* outwidth, u32* outheight, const struct krn_kernel* krnx, const struct krn_kernel* krny, u32 outstride);
