#pragma once

void lzoInit();
int lzoDecompress(char* cmpBuf, char* uncmpBuf, int* lzoRetLen, int compressedSize);