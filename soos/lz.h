#pragma once
//#define LZ_DEBUG
size_t lzss_enc(const void* in, void* out, size_t insize, size_t outsize);
size_t lzss_dec(const void* in, void* out, size_t insize);
