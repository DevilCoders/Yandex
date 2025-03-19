#pragma once

#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <kernel/keyinv/hitlist/invsearch.h>

#include "indexutil.h"

namespace NIndexerCore {
    class TInputIndexFile;
}

// Start points to the target buffer, Tail may point anywhere
// StartLen - number of valid chars in start
class TYndKeyText {
private:
    char* const Start;
    size_t StartLen;
    const char* Tail;
public:
    explicit TYndKeyText(char* targetBuffer);
    void AddTail();
    int Cmp(const char* p) const;
    const char* UnpackKeyText(const char* p);
    const char* UnpackKey(YxRecord &rec, const char *data);
};

class TFastAccessTable
{
private:
    bool IsCompressed;
    size_t BlockSize;
    TVector<NIndexerCore::TKeyBlockInfo> Infos;
    TBlob FAT;
    int FirstBlocks[256];

    TIndexInfo Reset(IYndexStorage::FORMAT format, ui32 version, bool hasSubIndex);
public:
    TFastAccessTable() {
        memset(FirstBlocks, 0, sizeof(FirstBlocks));
    }
    void Clear() {
        Infos.clear();
        FAT = TBlob();
        IsCompressed = false;
    }
    /// Reads FAT from index
    TIndexInfo Open(NIndexerCore::TInputIndexFile& indexFile);

    /// Reads FAT from stream
    TIndexInfo Open(NIndexerCore::TMemoryMapStream& invStream, ui32 version, const NIndexerCore::TInvKeyInfo& invKeyInfo);

    /// Reads FAT from memory
    TIndexInfo Open(const TBlob& inv, ui32 version, const NIndexerCore::TInvKeyInfo& invKeyInfo);

    ui32 Count() const {
        return (ui32)Infos.size();
    }
    bool Compressed() const {
        return IsCompressed;
    }
    ui32 KeyCount() const {
        return Infos.size() ? Infos.back().KeyCount : 0;
    }
    ui64 GetOffset(ui32 block) const {
        return Infos[block].Offset;
    }
    size_t GetKeyBlockSize() const {
        return BlockSize;
    }
    ui64 GetKeyOffset(ui32 block) const {
        return Infos[block].KeyOffset;
    }
    int FirstKeyInBlock(ui32 block) const {
        if (block > 0 && (ui32)block <= Infos.size())
            return (int)Infos[block-1].KeyCount;
        return 0;
    }

    const char* FirstKeyValueInBlock (ui32 block) {
        if (block > 0 && (ui32)block <= Infos.size())
            return Infos[block - 1].FirstKey;
        return nullptr;
    }
    i32 BlockByWord(const char *word) const;
    void GetBlock(i32 number, i32 &block) const;
};

class TFileMap;
i32 FatLowerBound(const TFastAccessTable&, const TIndexInfo&, const TFileMap&, const char *word, TRequestContext &rc);
const YxRecord* GetEntryByNumber(const TFastAccessTable&, const TIndexInfo&, const TFileMap&, TRequestContext &rc, i32 number, i32 &block);
