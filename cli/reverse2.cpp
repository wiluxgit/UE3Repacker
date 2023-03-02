// reverse2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <exception>
#include <cassert>
#include <stdio.h>
#include <wchar.h>
#include <string.h>

#include "reverse2.h"
#include "upkunpack.h"
#include "lzo.h"

using namespace std;

void* thmalloc(size_t Size) {
    void* ret = malloc(Size);
    assert(ret != NULL);
    return ret;
}
void* crashTODO(const char* s) {
    char buf[256];
    snprintf(ARRAY_ARG(buf), "TODO ERROR: %s", s);
    printf("%s\n", buf);
    throw new exception(buf);
    return NULL;
}
void extractFilenameWithoutEnding(wchar_t* buf, int buflen, wchar_t* src) {
    wchar_t* afterSlash = wcsrchr(src, '\\') + 1;
    wchar_t* indexOfDot = wcsrchr(afterSlash, '.');
    int len = indexOfDot - afterSlash;
    wcsncpy_s(buf, buflen, afterSlash, len);
}
char* ReadUPKInfo(FILE* upkFHandle, UE_header* ueHeader) {
    fread(ueHeader, 1, 0x10, upkFHandle);
    
    if (ueHeader->packageFileVersion == 0) {
        return (char*) -1;
    }

    //extract name to elsewhere i memory
    int nameLen = ueHeader->folderNameSize;
    char* headerBuf = (char*)thmalloc(nameLen);
    fread(headerBuf, 1, nameLen, upkFHandle);
    ueHeader->FXfolderName = headerBuf;

    //read known headers (up to and including dependOffset)
    fread(&ueHeader->packageFlags, 1, 0x20, upkFHandle);
    ueHeader->CChunk1UNOffset = (char*)ueHeader->nameOffset;
    ueHeader->totalChunkCount = 0x10;
    ueHeader->CChunk1UNSize = 0;

    long nRead = ftell(upkFHandle);
    //find end of header
    {
        char* remaingBytesOfHeaderCopy = (char*)thmalloc(0x40);
        fread(remaingBytesOfHeaderCopy, 1, 0x40, upkFHandle);
        int headerEndFinder = 0x14;
        do {
            int upper = *((int*)&remaingBytesOfHeaderCopy[headerEndFinder+4]);
            int lower = *((int*)&remaingBytesOfHeaderCopy[headerEndFinder]);
            if (upper == ueHeader->nameCount && lower == ueHeader->exportCount) {
                ueHeader->totalChunkCount = headerEndFinder + -4;
                break;
            }
            headerEndFinder = headerEndFinder + 1;
        } while (headerEndFinder < 0x39);
        free(remaingBytesOfHeaderCopy);
    }
    fseek(upkFHandle, nRead, 0);
    printf("ReadUPKInfo -> chunkCount = %x\n", ueHeader->totalChunkCount);

    // black magic
    int nChunks = ueHeader->totalChunkCount;
    char* chunkPointers = (char*)thmalloc(nChunks);

    ueHeader->FXidk_ImportExportGuidsOffset = chunkPointers;
    fread(chunkPointers, 1, nChunks, upkFHandle);
    fread(&ueHeader->idkImportGuidsCount, 1, 0x20, upkFHandle);

    // ?
    char* locUpk = (char*)ftell(upkFHandle);
    ueHeader->CChunk1UNSize = 0;
    if ((ueHeader->compressionFlag != 2) && (locUpk < ueHeader->CChunk1UNOffset)) {
        int chunkUNCmpressSize = (int)ueHeader->CChunk1UNOffset - (int)locUpk;
        ueHeader->CChunk1UNSize = chunkUNCmpressSize;
        char* uncompressAlloc = (char*)thmalloc(chunkUNCmpressSize);
        ueHeader->FXCompressedChunks = uncompressAlloc;
        fread(uncompressAlloc, 1, chunkUNCmpressSize, upkFHandle);
    }
    return ueHeader->CChunk1UNOffset;
}
void freeUEHeaderTables(UE_header* ueHeader) {
    int eti = 0;
    if ((ueHeader->FXExportTable != NULL) && (ueHeader->exportCount != 0)) {
        do {
            UE_subExport* xi = ueHeader->FXExportTable[eti].subExports;
            free(xi);
            eti += 1;
        } while (eti < ueHeader->exportCount);
    }
    free(ueHeader->FXExportTable);
    free(ueHeader->FXImportTable);
    free(ueHeader->FXNameTables);
    free(ueHeader->FXfolderName);
    free(ueHeader->FXidk_ImportExportGuidsOffset);
    free(ueHeader->FXCompressedChunks);
    free(ueHeader->CompressedHeaderTable);

    ueHeader->FXExportTable = NULL;
    ueHeader->FXImportTable = NULL;
    ueHeader->FXNameTables = NULL;
    ueHeader->FXfolderName = NULL;
    ueHeader->FXidk_ImportExportGuidsOffset = NULL;
    ueHeader->FXCompressedChunks = NULL;
    ueHeader->CompressedHeaderTable = NULL;
}
int writeNewHeader(UE_header* ueHeader, FILE* upkFHandle) {
    if (ueHeader->CChunk1UNOffset != NULL) {
        fwrite(ueHeader, 0x10, 1, upkFHandle);								     // tag, versions, headersize, namelen
        fwrite(ueHeader->FXfolderName, ueHeader->folderNameSize, 1, upkFHandle); // name
        fwrite(&ueHeader->packageFlags, 0x20, 1, upkFHandle);					 // Flags Name*2 Export*2 Import*2 Heritage
        fwrite(ueHeader->FXidk_ImportExportGuidsOffset, 0x20, 1, upkFHandle);    
        fwrite(&ueHeader->idkImportGuidsCount, 0x20, 1, upkFHandle);
        fwrite(ueHeader->FXCompressedChunks, 1, ueHeader->CChunk1UNSize, upkFHandle); // no idea
        return 0;
    }
    return -1;
}

void exitUsage() {
    printf("Usage:\n");
    printf("  upkrepacker unpack <path\\to\\file.upk> <out\\directory> [-x]\n");
    printf("  upkrepacker repack <in\\directory> [<out\\file.upk>] [-x]\n");
    printf("Info:\n");
    printf("  -x  Create additional log files\n");
    exit(1);
}
char* getElseExitUsage(char* arr[], int arrLen, int index) {    
    if (index >= arrLen) {
        printf("[[Parse Error]]: tried to read arg%d but only %d exist.\n\n", index, arrLen-1);
        exitUsage();
    }
    return arr[index];
}

// MAIN
int main(int argc, char* argv[]) {

    wchar_t inBuf[512];
    wchar_t outBuf[512];
    size_t nconv;

    bool doUnpack;
    bool doExtraLog = false;

    char* a1 = getElseExitUsage(argv, argc, 1);
    char* a2 = getElseExitUsage(argv, argc, 2);

    // parsing
    if (strcmp("unpack", a1) == 0) {
        doUnpack = true;
        mbstowcs_s(&nconv, ARRAY_ARG(inBuf), a2, 512);

        char* a3 = getElseExitUsage(argv, argc, 3);
        mbstowcs_s(&nconv, ARRAY_ARG(outBuf), a3, 512);
    }
    else if (strcmp("repack", a1) == 0) {
        doUnpack = false;
        mbstowcs_s(&nconv, ARRAY_ARG(inBuf), a2, 512);
        if (argc < 4) {
            outBuf[0] = L'\0';
        }
        else {
            char* a3 = getElseExitUsage(argv, argc, 3);
            mbstowcs_s(&nconv, ARRAY_ARG(outBuf), a3, 512);
        }
    }
    else {
        exitUsage();
    }

    // -x
    if (argc > 4) {
        char* a4 = getElseExitUsage(argv, argc, 4);
        if (strcmp("-x", a4)) {
            doExtraLog = true;
        }
    }

    lzoInit();

    if (doUnpack) {
        UPKunpack(
            inBuf,
            outBuf,
            (wchar_t*)L"",
            doExtraLog
        );
    }
    else {
        UPKrepack(
            inBuf,
            outBuf,
            (wchar_t*)L"",
            doExtraLog
        );
    }
    return 0;
}