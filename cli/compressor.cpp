#include <exception>
#include <cassert>
#include <iostream>
#include <windows.h>

#include "reverse2.h"
#include "compressor.h"
#include "lzo.h"

UE_CompHeadTable* getNthCMPH(UE_header* ueHeader, uint idx) {
	if (
		(ueHeader->nCompressedHeaders != 0)
		&& (-1 < idx)
		&& (idx <= ueHeader->nCompressedHeaders)
		) {
		return ueHeader->CompressedHeaderTable + idx;
	}
	return NULL;
}

UE_CompHeadTable* getIth(UE_header* ueHeader, uint tableIndex) {
	if ((ueHeader->nCompressedHeaders == 0) || (ueHeader->nCompressedHeaders < (uint)tableIndex)) {
		return NULL;
	}
	else {
		return ueHeader->CompressedHeaderTable + tableIndex;
	}
}

wchar_t* getCOMPBLOCKpath() {
	static wchar_t tempPath[512];
	GetTempPathW(512, tempPath);
	static wchar_t tempDecompPath[512];
	_snwprintf_s(ARRAY_ARG(tempDecompPath), L"%scompblock", tempPath);
	return tempDecompPath;
}

int copyCMPHbyIdToNewFile(uint tableIndex, FILE* upkFhandle, UE_header* ueHeader) {
	UE_CompHeadTable* ithTable = NULL;

	if ((int)tableIndex > -1) {
		int nCmps = ueHeader->nCompressedHeaders;
		if (tableIndex <= nCmps) {
			if (nCmps == 0) return NULL;

			ithTable = getIth(ueHeader, tableIndex);;

			char* dstBuf = (char*)thmalloc(ithTable->compSize); //replacing malloc check with crash

			wchar_t* tempDecompPath = getCOMPBLOCKpath();

			ithTable = getIth(ueHeader, tableIndex);

			fseek(upkFhandle, ithTable->compOffset, 0);
			fread(dstBuf, 1, ithTable->compSize, upkFhandle);

			ithTable = getIth(ueHeader, tableIndex);

			FILE* decompFHandle;
			_wfopen_s(&decompFHandle, tempDecompPath, L"wb");
			assert(decompFHandle != NULL);

			ithTable = getIth(ueHeader, tableIndex);

			assert(fwrite(dstBuf, 1, ithTable->compSize, decompFHandle) != 0);
			fclose(decompFHandle);
			free(dstBuf);
			return 0; //stack cookie check
		}
	}
	return 0; //stack cookie check
}

int decompressFile(FILE* FHandle) {
	wchar_t* cmpPath = getCOMPBLOCKpath();
	FILE* compFHandle;
	_wfopen_s(&compFHandle, cmpPath, L"rb");
	assert(compFHandle != NULL);

	// get block headers
	UE_CompHeadTable compHeader;
	fread(&compHeader,0x10,1,compFHandle);

	static UE_BlockHeaderItem mem[2500];

	int nChunks = 0;
	uint totalCompSize = 0;
	UE_BlockHeaderItem* datptr = (UE_BlockHeaderItem*)mem;
	do {
		fread(&datptr->comp, 4, 1, compFHandle);
		fread(&datptr->uncomp, 4, 1, compFHandle);
		totalCompSize += datptr->comp;
		nChunks = nChunks + 1;
		datptr = datptr + 1;
	} while (totalCompSize < compHeader.compOffset);

	char* cmpBuf = (char*)thmalloc(compHeader.unCompSize * 2);
	char* uncmpBuf = (char*)thmalloc(compHeader.unCompSize * 0x32);
	int doneChunks = 0;

	doneChunks = 0;
	if (nChunks != 0) {
		do {
			fread(cmpBuf, 1, mem[doneChunks].comp, compFHandle);
			if (mem[doneChunks].comp == mem[doneChunks].uncomp) {
				fwrite(cmpBuf, 1, mem[doneChunks].uncomp, FHandle);
			}
			else {
				int lzoRetLen;
				int compressedSize = mem[doneChunks].comp;
				lzoDecompress(cmpBuf, uncmpBuf, &lzoRetLen, compressedSize);
				fwrite(uncmpBuf, 1, lzoRetLen, FHandle);
			}
			doneChunks = doneChunks + 1;
		} while (doneChunks < nChunks);
	}
	fclose(compFHandle);
	free(cmpBuf);
	free(uncmpBuf);
	return 0;
}

int decompress(wchar_t* compressionResultPath, FILE* upkFHandle, UE_header* ueHeader) {
	int nCmph = ueHeader->compressedChunkCount;
	if (nCmph == 0) {
		return 0;
	}

	// alloc cmphTables
	UE_CompHeadTable* cmphTables = (UE_CompHeadTable*)calloc(nCmph, sizeof(UE_CompHeadTable));
	assert(cmphTables != NULL);
	ueHeader->CompressedHeaderTable = cmphTables;
	fread(cmphTables, sizeof(UE_CompHeadTable), nCmph, upkFHandle);
	ueHeader->nCompressedHeaders = nCmph;

	FILE* upkFHandleCopy = upkFHandle; //try removing it eventually, seems useless

	// Open compress result file
	FILE* decompFHandle;
	_wfopen_s(&decompFHandle, compressionResultPath, L"wb");
	assert(decompFHandle != NULL);

	// fix flags?
	ueHeader->packageFlags &= 0x00FFFFFF; //Could be wrong
	ueHeader->compressionFlag = 0;
	ueHeader->compressedChunkCount = 0;

	writeNewHeader(ueHeader, decompFHandle);

	// transfer boring header stuff
	long whereInUpkFile = ftell(upkFHandle);
	int remaingCharsToCopyToItemFileHeader = ueHeader->dependOffset - whereInUpkFile;
	if (0 < remaingCharsToCopyToItemFileHeader) {
		do {
			char mv;
			fread(&mv, 1, 1, upkFHandleCopy);
			fwrite(&mv, 1, 1, decompFHandle);
			remaingCharsToCopyToItemFileHeader -= 1;
		} while (remaingCharsToCopyToItemFileHeader != 0);
	}

    int ics = 0;
	int nCMPHs = ueHeader->nCompressedHeaders;
    if (ueHeader->nCompressedHeaders != 0) {
        UE_CompHeadTable* nextCMPH_A = NULL;
        UE_CompHeadTable* nextCMPH_B = NULL;
		UE_CompHeadTable* nextCMPH_C = NULL;
        UE_CompHeadTable* getCMPH = NULL;
        do {
            if (((nCMPHs == 0) || (ics < 0)) || (nCMPHs < ics)) { //why is this code so bad?
                nextCMPH_A = NULL;
                nextCMPH_B = NULL;
            }
            else {
                nextCMPH_A = &ueHeader->CompressedHeaderTable[ics];
                nextCMPH_B = &ueHeader->CompressedHeaderTable[ics];
            }
			if (nextCMPH_A->compSize == nextCMPH_B->unCompSize) {
				getCMPH = getNthCMPH(ueHeader, ics);
				if (getCMPH->compSize != 0) {
					int jCMPHs = 0;
					do {
						char mv;
						fread(&mv, 1, 1, upkFHandle);
						fwrite(&mv, 1, 1, decompFHandle);
						jCMPHs = jCMPHs + 1;
						getCMPH = getNthCMPH(ueHeader, ics);
					} while (jCMPHs < getCMPH->compSize);
				}
			}
			else {
				int ok = copyCMPHbyIdToNewFile(ics, upkFHandle, ueHeader);
				assert(ok == 0);

				if (
					(ueHeader->nCompressedHeaders == 0) 
					|| (ics < 0) 
					|| (ueHeader->nCompressedHeaders < ics)
				) {
					nextCMPH_C = NULL;
				}
				else {
					nextCMPH_C = &ueHeader->CompressedHeaderTable[ics];
				}
				assert(nextCMPH_C != NULL);
				fseek(decompFHandle, nextCMPH_C->unCompOffset, 0); 

				upkFHandleCopy = decompFHandle; //pointless?
				ok = decompressFile(decompFHandle);
				assert(ok == 0);
			}
			ics = ics + 1;
        } while (ics < ueHeader->nCompressedHeaders);
    }
    fclose(decompFHandle);
    return 1; //stack cookie check
}