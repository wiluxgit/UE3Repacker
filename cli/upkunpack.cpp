#include <exception>
#include <cassert>
#include <iostream>
#include <direct.h>
#include <wchar.h>
#include <windows.h>

#include "reverse2.h"
#include "compressor.h"

static UE_MysteryStruct statmystery;
static UE_FXOutFileInfo outFI;

int ReadNameTable(FILE* upkFHandle, UE_header* ueHeader) {
    if (ueHeader->nameCount != 0) {

        UE_FXNameTable* newFXNameTable 
            = (UE_FXNameTable*)calloc(ueHeader->nameCount, sizeof(UE_FXNameTable));
        assert(newFXNameTable != NULL); //failed to calloc.
        ueHeader->FXNameTables = newFXNameTable;

        fseek(upkFHandle, ueHeader->nameOffset, 0);
        uint iter = 0;
        if (ueHeader->nameCount != 0) {
            do {
                fread(&ueHeader->FXNameTables[iter].nameLength, 4, 1, upkFHandle);
                int nameLen = ueHeader->FXNameTables[iter].nameLength;
                fread(&ueHeader->FXNameTables[iter].str, 1, nameLen, upkFHandle);
                fread(&ueHeader->FXNameTables[iter].flags1, 4, 1, upkFHandle);
                fread(&ueHeader->FXNameTables[iter].flags2, 4, 1, upkFHandle);
                iter += 1;
            } while (iter < ueHeader->nameCount);
        }
        return ueHeader->nameCount;
    }
    return -1;
}

int ReadImportTable(FILE* upkFHandle, UE_header* ueHeader) {
    if (ueHeader->importCount != 0) {

        UE_FXImportTable* newFXImportTable 
            = (UE_FXImportTable*)calloc(ueHeader->importCount, sizeof(UE_FXImportTable));
        assert(newFXImportTable != NULL); //failed to calloc.
        ueHeader->FXImportTable = newFXImportTable;

        fseek(upkFHandle, ueHeader->importOffset, 0);
        uint iter = 0;
        if (ueHeader->importCount != 0) {
            do {
                fread(&ueHeader->FXImportTable[iter].package, 4, 1, upkFHandle);
                fread(&ueHeader->FXImportTable[iter].iclass, 4, 1, upkFHandle);
                fread(&ueHeader->FXImportTable[iter].outer, 4, 1, upkFHandle);
                fread(&ueHeader->FXImportTable[iter].name, 4, 1, upkFHandle);
                fread(&ueHeader->FXImportTable[iter].bonus1, 4, 1, upkFHandle);
                fread(&ueHeader->FXImportTable[iter].bonus2, 4, 1, upkFHandle);
                fread(&ueHeader->FXImportTable[iter].bonus3, 4, 1, upkFHandle);
                iter += 1;
            } while (iter < ueHeader->importCount);
        }
        return ueHeader->importCount;
    }
    return -1;
}

int ReadExportTable(FILE* upkFHandle, UE_header* ueHeader) {
    if (ueHeader->exportCount != 0) {
        ulonglong tableSize = (ulonglong)(ueHeader->exportCount) * sizeof(UE_FXExportTable);

        uint newLen = (uint)-((int)(tableSize >> 0x20 != 0)) | (uint)tableSize;
        UE_FXExportTable* newTable = (UE_FXExportTable*)thmalloc(newLen); //newed in src, should not matter
        ueHeader->FXExportTable = newTable;
        fseek(upkFHandle, ueHeader->exportOffset, 0);

        //DEBUG
        //printf("fuckyAlloc => %x | %x -> %x\n", (uint)-((int)(tableSize >> 0x20 != 0)), (uint)tableSize, newLen);

        if (ueHeader->packageFileVersion == 805) {
            int i = 0;
            while (i < ueHeader->exportCount) {
                ueHeader->FXExportTable[i].nSubExports = 5;

                ulonglong subTablesSize = (ulonglong)(ueHeader->FXExportTable[i].nSubExports) * sizeof(UE_subExport);
                uint newSubLen = (uint)(-(int)(subTablesSize >> 0x20 != 0)) | (uint)subTablesSize;

                UE_subExport* newSubDat = (UE_subExport*)thmalloc(newSubLen);
                ueHeader->FXExportTable[i].subExports = newSubDat;

                fread(ueHeader->FXExportTable[i].subExports, 4, ueHeader->FXExportTable[i].nSubExports, upkFHandle);
                ueHeader->FXExportTable[i].unpackedSize = ueHeader->FXExportTable[i].idkX24;
                ueHeader->FXExportTable[i].beginOfPkgInfoData = ueHeader->FXExportTable[i].idkX28;

                i += 1;
            }
        }
        else if (ueHeader->packageFileVersion < 537) {
            int i = 0;
            while (i < ueHeader->exportCount) {
                fread(&ueHeader->FXExportTable[i], 0x34, 1, upkFHandle); //0x34 (52) bytes = real upk data
                ueHeader->FXExportTable[i].nSubExports = 5;
                int idkX28 = ueHeader->FXExportTable[i].idkX28;
                if (idkX28 != 0) {
                    ueHeader->FXExportTable[i].nSubExports += idkX28 * 3;
                }
                if (ueHeader->FXExportTable[i].idkX30 != 0 && ueHeader->FXExportTable[i].idkX28 != 0) {
                    ueHeader->FXExportTable[i].nSubExports += ueHeader->FXExportTable[i].idkX30;
                }
                ulonglong subTablesSize = (ulonglong)(ueHeader->FXExportTable[i].nSubExports) * sizeof(UE_subExport);
                uint newSubLen = (uint)(-(int)(subTablesSize >> 0x20 != 0)) | (uint)subTablesSize;

                UE_subExport* newSubDat = (UE_subExport*)thmalloc(newSubLen);
                ueHeader->FXExportTable[i].subExports = newSubDat;

                fread(ueHeader->FXExportTable[i].subExports, 4, ueHeader->FXExportTable[i].nSubExports, upkFHandle);
                ueHeader->FXExportTable[i].unpackedSize = ueHeader->FXExportTable[i].idkX20;
                ueHeader->FXExportTable[i].beginOfPkgInfoData = ueHeader->FXExportTable[i].idkX24;

                i += 1;
            }
        }
        else {
            int i = 0;
            while (i < ueHeader->exportCount) {
                fread(&ueHeader->FXExportTable[i], 0x34, 1, upkFHandle); //0x34 (52) bytes = real upk data
                ueHeader->FXExportTable[i].nSubExports = 4;
                ueHeader->FXExportTable[i].nSubExports += ueHeader->FXExportTable[i].idkX2c;

                ulonglong subTablesSize = (ulonglong)(ueHeader->FXExportTable[i].nSubExports) * sizeof(UE_subExport);
                uint newSubLen = (uint)(-(int)(subTablesSize >> 0x20 != 0)) | (uint)subTablesSize;

                UE_subExport* newSubDat = (UE_subExport*)thmalloc(newSubLen); 
                ueHeader->FXExportTable[i].subExports = newSubDat;
                
                //depends on format
                fread(ueHeader->FXExportTable[i].subExports, 4, ueHeader->FXExportTable[i].nSubExports, upkFHandle);
                ueHeader->FXExportTable[i].unpackedSize = ueHeader->FXExportTable[i].idkX20;
                ueHeader->FXExportTable[i].beginOfPkgInfoData = ueHeader->FXExportTable[i].idkX24;

                i += 1;
            }
        }
        return ueHeader->exportCount;
    }
    return -1;
}

void writePKGInfo(UE_header* ueHeader, FILE* pkgInfoFHandle) {
    writeNewHeader(ueHeader, pkgInfoFHandle);

    //Name Table
    if ((ueHeader->nameCount != 0) && (ueHeader->FXNameTables != NULL)) {
        fseek(pkgInfoFHandle, ueHeader->nameOffset, 0);
        int nti = 0;
        if (ueHeader->nameCount != 0) {
            do {
                int nameLen = ueHeader->FXNameTables[nti].nameLength;
                fwrite(&ueHeader->FXNameTables[nti].nameLength, 4, 1, pkgInfoFHandle);
                fwrite(&ueHeader->FXNameTables[nti].str, 1, nameLen, pkgInfoFHandle);
                fwrite(&ueHeader->FXNameTables[nti].flags1, 4, 1, pkgInfoFHandle);
                fwrite(&ueHeader->FXNameTables[nti].flags2, 4, 1, pkgInfoFHandle);
                nti += 1;
            } while (nti < ueHeader->nameCount);
        }
    }

    //Import Table
    if ((ueHeader->importCount != 0) && (ueHeader->FXImportTable != NULL)) {
        fseek(pkgInfoFHandle, ueHeader->importOffset, 0);
        int iti = 0;
        if (ueHeader->importCount != 0) {
            do {
                fwrite(&ueHeader->FXImportTable[iti].package, 4, 1, pkgInfoFHandle);
                fwrite(&ueHeader->FXImportTable[iti].iclass, 4, 1, pkgInfoFHandle);
                fwrite(&ueHeader->FXImportTable[iti].outer, 4, 1, pkgInfoFHandle);
                fwrite(&ueHeader->FXImportTable[iti].name, 4, 1, pkgInfoFHandle);
                fwrite(&ueHeader->FXImportTable[iti].bonus1, 4, 1, pkgInfoFHandle);
                fwrite(&ueHeader->FXImportTable[iti].bonus2, 4, 1, pkgInfoFHandle);
                fwrite(&ueHeader->FXImportTable[iti].bonus3, 4, 1, pkgInfoFHandle);
                iti += 1;
            } while (iti < ueHeader->importCount);
        }
    }

    //Export Table
    if ((ueHeader->exportCount != 0) && (ueHeader->FXExportTable != NULL)) {
        fseek(pkgInfoFHandle, ueHeader->exportOffset, 0);
        int eti = 0;
        if (ueHeader->exportCount != 0) {
            do {
                if (ueHeader->packageFileVersion == 805) {
                    ueHeader->FXExportTable[eti].idkX24 = ueHeader->FXExportTable[eti].unpackedSize;
                    ueHeader->FXExportTable[eti].idkX28 = ueHeader->FXExportTable[eti].beginOfPkgInfoData;
                }
                else {
                    ueHeader->FXExportTable[eti].idkX20 = ueHeader->FXExportTable[eti].unpackedSize;
                    ueHeader->FXExportTable[eti].idkX24 = ueHeader->FXExportTable[eti].beginOfPkgInfoData;
                }
                fwrite(&ueHeader->FXExportTable[eti], 0x34, 1, pkgInfoFHandle); //0x34 (52) bytes = real upk data

                //subdata
                int nSubExport = ueHeader->FXExportTable[eti].nSubExports;
                if (nSubExport != 0) {
                    fwrite(ueHeader->FXExportTable[eti].subExports, sizeof(UE_subExport), nSubExport, pkgInfoFHandle);
                }

                eti += 1;
            } while (eti < ueHeader->exportCount);
        }
    }
}

void makePKGInfo(UE_header* ueHeader) {
    FILE* pkgInfoFhandle;
    _wfopen_s(&pkgInfoFhandle, L".UPKpkginfo", L"wb");
    assert(pkgInfoFhandle != NULL);
    writePKGInfo(ueHeader, pkgInfoFhandle);
    fclose(pkgInfoFhandle);
}

char* negativeClassNameGetter(UE_header* ueHeader, int invertClassNum) {
    if (
        (ueHeader->importCount != 0 
            && invertClassNum > -1)
            && (uint)invertClassNum <= ueHeader->importCount
    ) {
        int idk = ueHeader->FXImportTable[invertClassNum].bonus2;
        if (
            ueHeader->nameCount != 0 
                && -1 < (int)idk
                && idk <= ueHeader->nameCount
        ) {
            return ueHeader->FXNameTables[idk].str;
        }
    }
    return NULL;
}

char* getClassString(UE_header* ueHeader, int classAsNum) {
    static char STR_Class[5] = { 'C', 'l', 'a', 's','s' };
    static char STR_Null[6] = { '(', 'n', 'u', 'l', 'l',')' };
    if (classAsNum == 0) {
        return STR_Class;
    }
    if (classAsNum < 1) {
        if (classAsNum < 0 && (uint)-classAsNum < ueHeader->importCount) {
            return negativeClassNameGetter(ueHeader, -(1 + classAsNum));
        }
        return STR_Null;
    }
    int nameCount = ueHeader->nameCount;
    if (nameCount <= (uint)classAsNum) {
        return STR_Null;
    }
    int ntIndex = ueHeader->FXExportTable[classAsNum + -1].nameNTindex;
    if (
        nameCount != 0
            && -1 < (int)ntIndex
            && ntIndex <= nameCount
    ) {
        return ueHeader->FXNameTables[ntIndex].str;
    }
    return NULL;
}

void exportETpart(int exportIndex, UE_header* ueHeader, char* retBuf) {
    static char locBuf[512];

    UE_FXExportTable* fxExport = ueHeader->FXExportTable;
    for (int i = fxExport[exportIndex].outer; i != 0; i = fxExport[i-1].outer) {

        int nameCount = ueHeader->nameCount;
        int lastName = fxExport[i-1].nameNTindex;
        char* ntString = NULL;

        if (fxExport[i-1].nOverlapping == 0) {
            if (nameCount == 0 || (int)lastName < 0 || nameCount < lastName) {
                ntString = NULL;
            }
            else {
                ntString = ueHeader->FXNameTables[lastName].str;
            }
            sprintf_s(locBuf, 0x200, "%s\\%s", ntString, retBuf);
        }
        else {
            if (((nameCount == 0) || ((int)lastName < 0)) || (nameCount < lastName)) {
                ntString = (char*)0x0;
            }
            else {
                ntString = ueHeader->FXNameTables[lastName].str;
            }
            sprintf_s(locBuf, 0x200, "%s_%d\\%s", ntString, fxExport[i-1].nOverlapping - 1, retBuf);
        }
        sprintf_s(retBuf, 0x200, "%s", locBuf);
        fxExport = ueHeader->FXExportTable;
    }
}

void getNonOverlappingNameExport(UE_header* ueHeader, uint etidx, char* ret, int retLen) {
    if (etidx < ueHeader->exportCount || etidx == ueHeader->exportCount) {
        static char exportStr[512];
        memset(&exportStr, 0, 512);

        const char* classString = getClassString(ueHeader, ueHeader->FXExportTable[etidx].iclass);
        int ntIndex = ueHeader->FXExportTable[etidx].nameNTindex;

        char* tableName = NULL;
        if (
            ueHeader->nameCount != 0
                && -1 < (int)ntIndex
                && ntIndex <= ueHeader->nameCount
        ) {
            tableName = ueHeader->FXNameTables[ntIndex].str;
        }
        exportETpart(etidx, ueHeader, exportStr);

        static char strBuf[256];
        int nOverlapping = ueHeader->FXExportTable[etidx].nOverlapping;
        if (nOverlapping == 0) {
            sprintf_s(strBuf, 0xff, "%s.%s", tableName, classString);
        }
        else {
            sprintf_s(strBuf, 0xff, "%s_%d.%s", tableName, nOverlapping + -1, classString);
        }

        if (ueHeader->FXExportTable[etidx].outer == 0) {
            sprintf_s(ret, retLen, "%s", strBuf);
        }
        else {
            sprintf_s(ret, retLen, "%s%s", exportStr, strBuf);
        }
    }
}

wchar_t* getEndOfWString(wchar_t* str) {
    wchar_t wVar1;
    do {
        wVar1 = *str;
        str += 1;
    } while (wVar1 != L'\0');
    return str;
}

int wRelCharLen(wchar_t* end, wchar_t* start) {
    return ((int)end - (int)(start + 1)) / sizeof(wchar_t);
}

int createPackItemDir(wchar_t* path) {
    static wchar_t wbuf[512];
    wchar_t* endOf_path = getEndOfWString(path);

    if (wRelCharLen(endOf_path, path) == 0) {
        return 0;
    }

    memset(wbuf, 0, sizeof(wbuf));

    if (path[0] != L'\0') {
        int idx = 0;
        do {
            wbuf[idx] = path[idx];
            if (path[idx] == L'\\') {
                CreateDirectory(wbuf,0);
            }
            idx += 1;
        } while (path[idx] != L'\0');
    }
    CreateDirectory(wbuf, 0);
    return 0;
}

void XWARNING() {
    crashTODO("Why does it not understand?");
}

void logNameTable(UE_header* ueHeader) {
    FILE* logf;
    _wfopen_s(&logf, L"NameTable.Log", L"w");
    assert(logf != NULL);

    for (int i = 0; i < ueHeader->nameCount; i++) {
        UE_FXNameTable* currNT = &ueHeader->FXNameTables[i];

        fprintf(logf, "%d:%s Type:%08X Offset:%08X\n", i, currNT->str, currNT->flags1, currNT->flags2);
    }
    fclose(logf);
}
void logImportTable(UE_header* ueHeader) {
    FILE* logf;
    _wfopen_s(&logf, L"ImportTable.Log", L"w");
    assert(logf != NULL);

    for (size_t i = 0; i < ueHeader->importCount; i++) {
        UE_FXImportTable* currIT = &ueHeader->FXImportTable[i];

        char* bonusname = ueHeader->FXNameTables[currIT->bonus2].str;
        char* outername = ueHeader->FXNameTables[currIT->outer].str;
        char* className = negativeClassNameGetter(ueHeader, -1 - currIT->bonus1);

        fprintf(logf, "%d ", i);
        fprintf(logf, "%s.%s.%s", outername, className, bonusname);
        fprintf(logf, " %d %d %d %d %d %d\n",
            currIT->package, currIT->iclass, currIT->outer, currIT->name, 
            currIT->bonus1, currIT->bonus2, currIT->bonus3
        );
    }
    fclose(logf);
}
void logExportTable(UE_header* ueHeader) {
    FILE* logf;
    _wfopen_s(&logf, L"ExportTable.Log", L"w");
    assert(logf != NULL);

    static char ETRet[1040];
    for (int ii = 0; ii < ueHeader->exportCount; ii++) {
        UE_FXExportTable* currET = &ueHeader->FXExportTable[ii];

        getNonOverlappingNameExport(ueHeader, ii, ARRAY_ARG(ETRet));
        fprintf(logf, "%d %s Size:%08x Offset:%08x ", ii, ETRet, currET->unpackedSize, currET->beginOfPkgInfoData);
        fprintf(logf, "%03d %03d %03d %03d %08x", currET->iclass, currET->super, currET->outer, currET->nameNTindex, currET->nOverlapping);
        fprintf(logf, "%08x %08x %08x %08x %08x %08x %08x %08x"
            , *(int*)&currET->flags1, *(int*)&currET->flags2
            , *(int*)&currET->serialSize, *(int*)&currET->idkX20
            , *(int*)&currET->idkX24, *(int*)&currET->idkX28
            , *(int*)&currET->idkX2c, *(int*)&currET->idkX30
        );

        for (int j = 0; j < currET->nSubExports; j++) {
            fprintf(logf, " %08x", currET->subExports[j]);
        }
        fprintf(logf, "\n");
    }
    fclose(logf);
}

void dumpExports(UE_header* ueHeader, wchar_t* outDirUpk, FILE* upkFHandle) {
    outFI.ExportTableidx = 0;
    uint iii = 1;
    bool bDone = 0;
    do {
        if (outFI.ueHeader.exportCount == 0 || outFI.ueHeader.exportCount < iii - 1) {
            outFI.currFXExport = NULL;
        }
        else {
            outFI.currFXExport = &ueHeader->FXExportTable[outFI.ExportTableidx];
        }

        static char etret[1040];
        static wchar_t w_etret[1040];
        getNonOverlappingNameExport(ueHeader, iii - 1, ARRAY_ARG(etret));
        mbstowcs_s(&outFI.idkX1c, ARRAY_ARG(w_etret), ARRAY_ARG(etret));

        wprintf(L"Unpacking(%d/%d):%s\n", iii, outFI.ueHeader.exportCount, w_etret);

        static wchar_t itemName[260];
        static wchar_t itemVirtualDirectory[260];
        wchar_t* pathFromSlash = wcsrchr(w_etret, L'\\');
        if (pathFromSlash == NULL) {
            itemName[0] = L'\0';
            itemVirtualDirectory[0] = L'\0';
        }
        else {
            if (*pathFromSlash == L'\\') {
                pathFromSlash+=1;
            }
            wcsncpy_s(itemName, 260, pathFromSlash, 260);
            wcsnlen(itemName, 260); //uuh, why?

            int fnameLen = ((int)pathFromSlash - (int)w_etret) / sizeof(wchar_t);
            if (fnameLen <= 260) {
                wcsncpy_s(itemVirtualDirectory, 260, w_etret, fnameLen);
            }
        }

        //Create Directories to dump stuff:
        wchar_t* endOf_outDirUpk = getEndOfWString(outDirUpk);
        wchar_t* endOf_w_etret = getEndOfWString(w_etret);

        int len_outDirUpk = wRelCharLen(endOf_outDirUpk, outDirUpk);
        int len_w_etret = wRelCharLen(endOf_w_etret, w_etret);

        bool doSpecialWrite = false;
        if (len_outDirUpk + len_w_etret + 1 >= 260) {
            doSpecialWrite = true;
        }
        if (doSpecialWrite == false) {
            int x = createPackItemDir(itemVirtualDirectory);
            doSpecialWrite = x < 0;
        }
        if (doSpecialWrite) {
            /* ?BIG PATH FIX? */
            swprintf_s(itemVirtualDirectory, 260, L".%d", iii);
            createPackItemDir(itemVirtualDirectory);
            wchar_t* postDot = wcsrchr(itemName, L'.');
            if (postDot == NULL) {
                outDirUpk[256] = L'\0';
            }
            else {
                if (postDot[0] == L'\\') {
                    postDot += 1;
                } //dumb path length shit?
                wcsncpy_s(outDirUpk + 0x100, 0x104, postDot, 0x104);
                wcsnlen(outDirUpk + 0x100, 0x104);
            }

            swprintf_s(w_etret, 0x104, L"%s\\%d%s", itemVirtualDirectory, iii, outDirUpk + 0x100);
            XWARNING();
            wprintf(L"Waring:Export File to %s\n", w_etret);
        }

        //Open Export stuff
        UE_FXExportTable* xOffset = outFI.currFXExport;
        char* unpackBuf = (char*)thmalloc(outFI.currFXExport->unpackedSize);
        fseek(upkFHandle, xOffset->beginOfPkgInfoData, 0);
        fread(unpackBuf, 1, xOffset->unpackedSize, upkFHandle);
        _wfopen_s(&outFI.currentEXFHandle, w_etret, L"wb");

        // if cant open?
        if (outFI.currentEXFHandle == NULL) {
            swprintf_s(itemVirtualDirectory, 0x104, L".%d", iii);
            createPackItemDir(itemVirtualDirectory);

            /* ?BIG PATH FIX? */
            wchar_t* remaX = wcsrchr(itemName, L'.');
            if (remaX == (wchar_t*)0x0) {
                outDirUpk[256] = L'\0';
            }
            else {
                if (*remaX == L'\\') {
                    remaX += 1;
                }
                wcsncpy_s(outDirUpk + 0x100, 0x104, remaX, 0x104);
                wcsnlen(outDirUpk + 0x100, 0x104); //uuh, why?
            }

            swprintf_s(w_etret, 0x104, L"%s\\%d%s", itemVirtualDirectory, iii, outDirUpk + 0x100);
            XWARNING();
            wprintf(L"!!Warning:Export File to %s\n", w_etret);

            _wfopen_s(&outFI.currentEXFHandle, w_etret, L"wb");
            if (outFI.currentEXFHandle == NULL) {
                wprintf(L"!!!!! ERROR !!!!!:Failed to open outputfile:%s\n", w_etret);

                crashTODO("Final export thingy does not work");
                return;
            }
        }

        fwrite(unpackBuf, 1, outFI.currFXExport->unpackedSize, outFI.currentEXFHandle);
        fclose(outFI.currentEXFHandle);
        free(unpackBuf);

        outFI.ExportTableidx += 1;
        bDone = iii < ueHeader->exportCount;
        iii += 1;
    } while (bDone);

    fclose(upkFHandle);
    printf("Done..\n");
}

void UPKrepack(wchar_t* srcDir, wchar_t* outArg, wchar_t* logPath, char enableXLog) {
    UE_header* ueHeader = &outFI.ueHeader;

    wchar_t origWorkingDir[512];
    assert(_wgetcwd(ARRAY_ARG(origWorkingDir)) != NULL);

    printf("UPKRepacking...\n");

    struct _stat64i32 srcStat[11];
    int dirOk = _wstat64i32(srcDir, srcStat);
    if (dirOk != 0) {
        wprintf(L"!!!!! ERROR !!!!!:Directory:%s is not found.\n", srcDir);
        crashTODO("");
    }

    wchar_t outUpk[512];
    if (outArg == NULL || outArg[0] == '\0') {
        swprintf_s(ARRAY_ARG(outUpk), L"%s.upk", srcDir);
    }
    else {
        swprintf_s(ARRAY_ARG(outUpk), L"%s", outArg);
    }
    wprintf(L"SorceDir:\t%s\n", srcDir);
    wprintf(L"DestFile:\t%s\n", outUpk);

    // Backup
    //wchar_t bakPath[512];
    //swprintf_s(bakPath, 0x104, L"%s.bak", outUpk);
    //MoveFileW(outUpk, bakPath);

    // Out UPK
    FILE* outFHandle;
    _wfopen_s(&outFHandle, outUpk, L"wb");
    assert(outFHandle != NULL);

    // UPKpkgInfo
    FILE* pkgInfoFHandle;
    wchar_t pkgInfoPath[512];
    swprintf_s(ARRAY_ARG(pkgInfoPath), L"%s\\%s", srcDir, L".UPKpkgInfo");
    _wfopen_s(&pkgInfoFHandle, pkgInfoPath, L"rb");
    assert(pkgInfoFHandle != NULL);

    wprintf(L"Read UPKinfo...\n");
    ReadUPKInfo(pkgInfoFHandle, ueHeader);
    wprintf(L"UPK Version:0x%x\n", ueHeader->packageFileVersion);
    wprintf(L"Read NameTables...\n");
    ReadNameTable(pkgInfoFHandle, ueHeader);
    wprintf(L"Read ImportTables...\n");
    ReadImportTable(pkgInfoFHandle, ueHeader);
    wprintf(L"Read ExportTables...\n");
    ReadExportTable(pkgInfoFHandle, ueHeader);

    fclose(pkgInfoFHandle);
    
    assert(_wchdir(srcDir) == 0);

    // begin
    int endOfExports = 0;
    if (ueHeader->exportCount == 0) {
        wprintf(L"!!!!! ERROR !!!!!:%s Has not exports\n", pkgInfoPath);
    } 
    else {

        // fix ueHeader and get data size
        endOfExports = ueHeader->FXExportTable[0].beginOfPkgInfoData;
        int eti = 0;
        do {
            UE_FXExportTable* currEx = &ueHeader->FXExportTable[eti];

            char readPath[512];
            wchar_t w_readPath[512];
            size_t nConvertedChars;
            getNonOverlappingNameExport(ueHeader, eti, ARRAY_ARG(readPath));
            mbstowcs_s(&nConvertedChars, ARRAY_ARG(w_readPath), ARRAY_ARG(readPath));
            
            //get class
            wchar_t classStr[512];
            wchar_t* pathFromDot = wcsrchr(w_readPath, L'.');
            if (pathFromDot == NULL) {
                classStr[0] = '\0';
            }
            else {
                if (*pathFromDot == L'\\') {
                    pathFromDot++;
                }
                wcsncpy_s(ARRAY_ARG(classStr), pathFromDot, 512); // dumb index
                wcsnlen(classStr, 0x104); // classic pointless
            }

            eti++;
            wchar_t backupPath[512];
            swprintf_s(ARRAY_ARG(backupPath), L".%d\\%d%s", eti, eti, classStr);

            struct _stat64i32 fileStat[11];
            int fstatOK = _wstat64i32(backupPath, fileStat);
            if (fstatOK != 0) {
                fstatOK = _wstat64i32(w_readPath, fileStat);
            }
            if (fstatOK != 0) {
                wprintf(L"!!!!! ERROR !!!!!:Can\'t find file:%s (%s)\n", w_readPath, backupPath);
                crashTODO("");
            }

            currEx->beginOfPkgInfoData = endOfExports;
            currEx->unpackedSize = fileStat->st_size;
            endOfExports += fileStat->st_size;
        } while (eti < ueHeader->exportCount);

        // temp
        wchar_t tempPath[512];
        writePKGInfo(ueHeader, outFHandle);
        GetTempPathW(512, tempPath);

        eti = 0;
        do {
            UE_FXExportTable* currEx = &ueHeader->FXExportTable[eti];

            wchar_t* fileToRead;
            char readPath[512];
            wchar_t w_readPath[512];
            size_t nConvertedChars;
            getNonOverlappingNameExport(ueHeader, eti, ARRAY_ARG(readPath));
            mbstowcs_s(&nConvertedChars, ARRAY_ARG(w_readPath), ARRAY_ARG(readPath));

            //get class
            wchar_t classStr[512];
            wchar_t* pathFromDot = wcsrchr(w_readPath, L'.');
            if (pathFromDot == NULL) {
                classStr[0] = '\0';
            }
            else {
                if (*pathFromDot == L'\\') {
                    pathFromDot++;
                }
                wcsncpy_s(ARRAY_ARG(classStr), pathFromDot, 512); // dumb index
                wcsnlen(classStr, 0x104); // classic pointless
            }

            eti++;
            wchar_t backupPath[512];
            swprintf_s(ARRAY_ARG(backupPath), L".%d\\%d%s", eti, eti, classStr);

            struct _stat64i32 fileStat[11];
                
            if (_wstat64i32(backupPath, fileStat) == 0) {
                fileToRead = backupPath;
            } else {
                if (_wstat64i32(w_readPath, fileStat) == 0) {
                    fileToRead = w_readPath;
                } else {
                    wprintf(L"!!!!! ERROR !!!!!:Can\'t find file:%s (%s)\n", w_readPath, backupPath);
                    crashTODO("");
                    exit(-1);
                }
            }

            wprintf(L"Writing (%d/%d) from %s\n", eti, ueHeader->exportCount, fileToRead);

            //copy data
            int _Count = fileStat->st_size;
            char* _DstBuf = (char*)thmalloc(fileStat->st_size);
            FILE* readFHandle;
            _wfopen_s(&readFHandle, fileToRead, L"rb");
            assert(readFHandle != NULL);

            fread(_DstBuf, 1, _Count, readFHandle);
            fclose(readFHandle);
            fseek(outFHandle, currEx->beginOfPkgInfoData, 0);
            fwrite(_DstBuf, 1, _Count, outFHandle);
            free(_DstBuf);

        } while (eti < ueHeader->exportCount);
    }
    fclose(outFHandle);
    wprintf(L"%s Has been successfully created\nDone\n", outUpk);
}

void UPKunpack(wchar_t* upkPath, wchar_t* outDir, wchar_t* logPath, char enableXLog) {
    UE_header* ueHeader = &outFI.ueHeader;


    //FILE* logFHandle;
    //_wfopen_s(&logFHandle, logPath, L"w");

    //struct _stat64i32 upkStat[11];
    //int bStatUpkOK = _wstat64i32(upkPath, upkStat);

    wchar_t wstrbuf[512];
    wchar_t tempDecompPath[512];
    wchar_t outDirUpk[512];

    wchar_t origWorkingDir[512];
    assert(_wgetcwd(ARRAY_ARG(origWorkingDir)) != NULL);
    wchar_t tempPath[512];
    assert(GetTempPath(512, tempPath) != NULL);

    printf("UPKUnpacking...\n");

    extractFilenameWithoutEnding(ARRAY_ARG(wstrbuf), upkPath);
    swprintf_s(ARRAY_ARG(outDirUpk), L"%s\\%s", outDir, wstrbuf);
    wprintf(L"DestDir:\t%s\n", outDirUpk);


    // Begin file handling
    FILE* upkFHandle;
    _wfopen_s(&upkFHandle, upkPath, L"rb");
    assert(upkFHandle != NULL);

    // Check (and fix) if whole file compression (aka not chunkwise compression)
    ReadUPKInfo(upkFHandle, ueHeader);
    if (ueHeader->packageFileVersion == 0) {
        wprintf(L"FullCompress UPK.Decompressing...\n");
        crashTODO("Fully compressed UPKs");
    }

    wprintf(L"UPK Version:%d (0x%x)\n", ueHeader->packageFileVersion, ueHeader->packageFileVersion);

    if ((ueHeader->packageFlags & 0xFF000000) == 0 || ueHeader->compressedChunkCount == 0) {
        GOTOBeginLoop:
        CreateDirectory(outDirUpk, 0);
        assert(_wchdir(outDirUpk) == 0);

        int nNames = ReadNameTable(upkFHandle, ueHeader);
        assert(nNames >= 0);
        printf("Read NameTable(%d)\n", ueHeader->nameCount);

        if (enableXLog) {
            logNameTable(ueHeader);
        }

        int nImports = ReadImportTable(upkFHandle, ueHeader);
        assert(nImports >= 0);
        printf("Read ImportTable(%d)\n", ueHeader->importCount);

        if (enableXLog) {
            logImportTable(ueHeader);
        }

        int nExports = ReadExportTable(upkFHandle, ueHeader);
        assert(nExports >= 0);
        printf("Read ExportTable(%d)\n", ueHeader->exportCount);

        if (enableXLog) {
            logExportTable(ueHeader);
        }

        makePKGInfo(ueHeader);
        if (ueHeader->exportCount != 0) {
            dumpExports(ueHeader, outDirUpk, upkFHandle);
        }

        // Cleanup
        fclose(upkFHandle);
        _wchdir(origWorkingDir);
        if (outFI.createdUncompressFile != '\0') {
            DeleteFile(tempDecompPath);
        }
        freeUEHeaderTables(ueHeader);
        exit(0);
    }
    else {
        if (ueHeader->compressionFlag != 2) {
            wprintf(L"Warning:Unknown CompressFlag %x\n", ueHeader->compressionFlag);
            goto GOTOBeginLoop;
        }

        _snwprintf_s(ARRAY_ARG(tempDecompPath), L"%s.decomp", tempPath);

        printf("Decompressing...\n");
        if (decompress(tempDecompPath, upkFHandle, ueHeader)) {
            //replace upkfhandle with uncompressed version
            fclose(upkFHandle); 
            _wfopen_s(&upkFHandle, tempDecompPath, L"rb");
            freeUEHeaderTables(ueHeader);
            ReadUPKInfo(upkFHandle, ueHeader); //re-read upk after decompression
            outFI.createdUncompressFile = 1;
            goto GOTOBeginLoop;
        }
        crashTODO("!!!! ERROR !!!!!:Decompressing Error!\n");
    }
}