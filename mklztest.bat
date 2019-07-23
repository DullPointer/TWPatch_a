@gcc -o lzenc.exe lztest.c soos/lz.c -DLZ_ENC
@gcc -o lzdec.exe lztest.c soos/lz.c -DLZ_DEC
@pause