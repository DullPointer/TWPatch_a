#pragma once

#include "red.h"

#define PAT_REDSHIFT    (1 << 0)
#define PAT_HOLE        (1 << 1)
#define PAT_RELOC       (1 << 2)
#define PAT_WIDE        (1 << 3)

void* memesearch(const void* patptr, const void* bitptr, const void* searchptr, size_t searchlen, size_t patsize);
size_t pat_copyhole(uint8_t* patchbuf, const color_setting_t* sets, size_t mask, size_t* outsize);
size_t pat_apply(uint8_t* codecptr, size_t codecsize, const color_setting_t* sets, size_t mask);
