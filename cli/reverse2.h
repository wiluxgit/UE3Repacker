#pragma once

#pragma pack(1) 

#define ARRAY_ARG(array)		array, sizeof(array)/sizeof(array[0])

typedef unsigned char    byte;
typedef unsigned short    word;
typedef unsigned int dword;
typedef unsigned long long qword;
typedef unsigned int    uint;
typedef unsigned short    ushort;
typedef unsigned long long ulonglong;
typedef unsigned int UE_subExport;

struct UE_FXNameTable {
    int nameLength;
    char str[1000];
    dword flags1;
    dword flags2;
};
struct UE_FXImportTable {
    dword package;
    dword iclass;
    dword outer;
    dword name;
    dword bonus1;
    dword bonus2;
    dword bonus3;
};
struct UE_FXExportTable {
    dword iclass;
    dword super;
    int outer;
    dword nameNTindex;
    int nOverlapping;
    dword flags1;
    dword flags2;
    dword serialSize;
    dword idkX20;
    dword idkX24;
    dword idkX28;
    dword idkX2c;
    dword idkX30;
    int nSubExports;
    UE_subExport* subExports;
    int unpackedSize;
    int beginOfPkgInfoData;
}; 
#define PREVIRESWWWDADAS sizeof(UE_FXExportTable)
struct UE_MysteryStruct {
    char idk0;
    char idk1;
    char idk2;
    char idk3;
    char idk4;
    char idk5;
    char idk6;
    char idk7;
    char idk8;
    char idk9;
    char idk10;
    char idk11;
    char idk12;
    char idk13;
    char idk14;
    char idk15;
    dword field16_0x10;
    dword field17_0x14;
    dword field18_0x18;
    dword field19_0x1c;
    FILE* file;
};
struct UE_CompHeadTable {
    int unCompOffset;
    int unCompSize;
    int compOffset;
    int compSize;
};
struct UE_header {
    dword tag;
    word packageFileVersion;
    word licenceVersion;
    dword headerSize;
    dword folderNameSize;
    char* FXfolderName;
    dword packageFlags;
    dword nameCount;
    dword nameOffset;
    int exportCount;
    dword exportOffset;
    dword importCount;
    dword importOffset;
    dword dependOffset;
    char* FXidk_ImportExportGuidsOffset;
    dword idkImportGuidsCount;
    dword idkExportGuidsCount;
    dword idkThumbnailTableOffset;
    dword idkPackageGUID;
    dword idkGenerationCount;
    dword idkGenerationInfo1;
    dword compressionFlag;
    dword compressedChunkCount;
    char* FXCompressedChunks;
    UE_FXNameTable* FXNameTables;
    UE_FXImportTable* FXImportTable;
    UE_FXExportTable* FXExportTable;
    int totalChunkCount;
    char* CChunk1UNOffset;
    dword CChunk1UNSize;
    int nCompressedHeaders;
    UE_CompHeadTable* CompressedHeaderTable;
};
struct UE_FXOutFileInfo {
    UE_FXExportTable* currFXExport;
    char idkX4;
    char idkX5;
    char idkX6;
    char createdUncompressFile;
    int compressSize;
    int decompressSize;
    int ExportTableidx;
    FILE* outDirWpathOrFHandle;
    FILE* currentEXFHandle;
    size_t idkX1c;
    struct UE_header ueHeader;
};
struct UE_BlockHeaderItem {
    int comp;
    int uncomp;
};

void extractFilenameWithoutEnding(wchar_t* buf, int buflen, wchar_t* src);
void* crashTODO(const char* s);
void* thmalloc(size_t Size);

char* ReadUPKInfo(FILE* upkFHandle, UE_header* ueHeader);
void freeUEHeaderTables(UE_header* ueHeader);
int writeNewHeader(UE_header* ueHeader, FILE* upkFHandle);