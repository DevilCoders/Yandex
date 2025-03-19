#include <util/memory/blob.h>
#include <util/system/file.h>
#include <kernel/search_types/search_types.h>
#include "indexutil.h"

namespace NIndexerCore {

    // const implies static
    const ui32 versionBuffer[8] = {
        0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF
    };

    const ui32 HAS_VERSION_FIX = 0xFFFFFFFF;

    bool ReadKeyText(TStreamBlock& keyStream, char* textBuffer) {
        int c = keyStream.GetChar();
        if (c == 0 || c == EOF)
            return false;

        int len = 0;
        if (1 <= c && c <= 31) {
            if (c == 1) {
                c = keyStream.GetChar();
                if (c <= 31 || c >= 256 || c == EOF)
                    ythrow yexception() << "Unexpected character at: " << keyStream.GetPosition() << " after key: [" << textBuffer << "]\n";
            }
            if (c > static_cast<int>(strlen(textBuffer)))
                ythrow yexception() << "Unexpected character at: " << keyStream.GetPosition() << " after key: [" << textBuffer << "]\n";
            len = (unsigned char)c;
        } else
            textBuffer[len++] = (char)c;

        while (len < MAXKEY_BUF) {
            c = keyStream.GetChar();
            if (c == 0 || c == EOF) {
                textBuffer[len] = 0;
                break;
            }
            if (c <= 0 || c >= 256)
                ythrow yexception() << "Unexpected character at: " << keyStream.GetPosition() << " after key: [" << textBuffer << "]\n";
            textBuffer[len++] = (char)c;
        }

        if (c != 0) {
            textBuffer[len - 1] = 0; // drop the last overflow char
            ythrow yexception() << "Key file is corrupt at: " << keyStream.GetPosition() << " after key: [" <<  textBuffer << "]\n";
        }

        return true;
    }

    void ReadIndexInfo(const TInputFile& file, ui32& version, TInvKeyInfo& invKeyInfo) {
        TFile invStream(file.GetName(), OpenExisting | RdOnly);
        ReadIndexInfoFromStream(invStream, version, invKeyInfo);
    }

    TBlob ReadFastAccessTable(const TInputFile& invStream, ui32 version, bool& isCompressed, TVector<TKeyBlockInfo>& infos, const TMemoryMap& keys, int firstBlocks[256]) {
        TFile file(invStream.GetName(), OpenExisting | RdOnly);
        return ReadFastAccessTable(file, version, isCompressed, infos, keys, firstBlocks);
    }

    TString PrintIndexVersion(ui32 version) {
        TString s;
        switch (version & YNDEX_VERSION_MASK) {
        case YNDEX_VERSION_NWORD_FORM:
            s = "YNDEX_VERSION_NWORD_FORM";
            break;
        case YNDEX_VERSION_RAW64_HITS:
            s = "YNDEX_VERSION_RAW64_HITS";
            break;
        case YNDEX_VERSION_BLK8:
            s = "YNDEX_VERSION_BLK8";
            break;
        default:
            s = "YNDEX_VERSION_UNKNOWN";
            break;
        }
        if (version & YNDEX_VERSION_FLAG_KEY_COMPRESSION & YNDEX_VERSION_FLAG_KEY_ADDITIONALY_COMPRESSED_YAPPY) {
            s += " YNDEX_VERSION_FLAG_KEY_ADDITIONALY_COMPRESSED_YAPPY";
        }
        if (version & YNDEX_VERSION_FLAG_KEY_COMPRESSION & YNDEX_VERSION_FLAG_KEY_2K) {
            s += " YNDEX_VERSION_FLAG_KEY_2K";
        }
        return s;
    }

    void FillFATBlocks(const TBlob& fat, TVector<TKeyBlockInfo>& infos, int firstBlocks[256], ui32 version, bool &isCompressed) {
        bool needKeyOffset = version & YNDEX_VERSION_FLAG_KEY_COMPRESSION;
        isCompressed = needKeyOffset;

        ui64 j = 0;
        const char* bufbeg = (const char*)fat.Data();
        size_t cEnter = (unsigned char)*bufbeg + 1;
        for (const char *p = bufbeg; (size_t)(p - bufbeg) < fat.Size();) {
            infos[j].FirstKey = p;
            while (cEnter <= (unsigned char)*p) {
                firstBlocks[cEnter] = j - 1;
                ++cEnter;
            }
            p += strlen(p) + 1;
            ui64 offset = 0;
            GET_64_OR(offset, p, ui64, 0);
            infos[j].Offset = offset;
            if (needKeyOffset) {
                offset = 0;
                GET_64_OR(offset, p, ui64, 0);
                infos[j].KeyOffset = offset;
            } else {
                infos[j].KeyOffset = j * KeyBlockSize(false);
            }
            ui32 keyCount = 0;
            GET_32_OR(keyCount, p, ui32, 0);
            infos[j].KeyCount = keyCount;
            ++j;
        }

        if (infos.size() != (size_t)j)
            ythrow yexception() << "fast access table corrupted";

        if (j > 1) {
            while (cEnter <= 255) {
                firstBlocks[cEnter] = j - 1;
                ++cEnter;
            }
        }
    }

    TBlob ReadFastAccessTable(const TBlob& blob, ui32 version, bool &isCompressed, TVector<TKeyBlockInfo>& infos, int firstBlocks[256]) {
        if (blob.Length() < (size_t) TInvKeyInfo::INFO_SIZE)
            ythrow yexception() << "incorrect inv blob size: " << blob.Length();

        TInvKeyInfo* invKeyInfo = (TInvKeyInfo*) (blob.AsCharPtr() + (blob.Length() - TInvKeyInfo::INFO_SIZE));

        if (!invKeyInfo->SizeOfFAT)
            return TBlob();

        i64 fastAccessOffset = blob.Length() - (invKeyInfo->SizeOfFAT + TInvKeyInfo::INFO_SIZE);

        TBlob fat = blob.SubBlob(fastAccessOffset, fastAccessOffset + invKeyInfo->SizeOfFAT);

        infos.resize(abs(invKeyInfo->NumberOfBlocks));

        FillFATBlocks(fat, infos, firstBlocks, version, isCompressed);
        return fat;
    }
}
