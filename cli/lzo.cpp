#include <windows.h>
#include <libloaderapi.h>
#include <cassert>
#include <iostream>

#include "reverse2.h"

typedef int (*lzo1x_decompress_T)(char* cmpBuf, int compressedSize, char* uncmpBuf, int* lzoRetLen, char* workmem);
typedef int (*__lzo_init_v2_T)(uint v, int s1, int s2, int s3, int s4, int s5, int s6, int s7, int s8, int s9);

lzo1x_decompress_T lzo1x_decompress;
__lzo_init_v2_T __lzo_init_v2;

void lzoInit() {
	auto myDLL = LoadLibrary(L"lzo2.dll");
	assert(myDLL != NULL);

	__lzo_init_v2 = (__lzo_init_v2_T)GetProcAddress(myDLL, "__lzo_init_v2");
	lzo1x_decompress = (lzo1x_decompress_T)GetProcAddress(myDLL, "lzo1x_decompress");

	assert(__lzo_init_v2 != NULL);
	assert(lzo1x_decompress != NULL);
}

// source: https://github.com/nemequ/lzo/blob/master/include/lzo/lzo1x.h
char _lzoWorkMemory[14 * 16384L * sizeof(short)];
int lzoDecompress(char* cmpBuf, char* uncmpBuf, int* lzoRetLen, int compressedSize) {
	return lzo1x_decompress(cmpBuf, compressedSize, uncmpBuf, lzoRetLen, _lzoWorkMemory);
}