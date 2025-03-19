#include <kernel/search_types/search_types.h>
#include <util/generic/algorithm.h>
#include <util/system/filemap.h>
#include <util/stream/output.h>

#include <kernel/keyinv/hitlist/longs.h>

#include "indexfile.h"
#include "fat.h"

using namespace NIndexerCore;

inline const char* UnpackFullKey(YxRecord &rec, const char *p) {
    int len;

    UNPACK_64(rec.Counter, p, mem_traits, len);
    Y_ASSERT(len);

    i64 l64;
    UNPACK_64(l64, p, mem_traits, len);
    Y_ASSERT(len);
    rec.Length = static_cast<ui32>(l64);
    Y_ASSERT(rec.Length == l64);
    Y_UNUSED(len);

    return p;
}

TYndKeyText::TYndKeyText(char* targetBuffer)
    : Start(targetBuffer)
    , StartLen(strlen(targetBuffer))
    , Tail(nullptr)
{
}

void TYndKeyText::AddTail() {
    if (Tail)
        strcpy(Start + StartLen, Tail);
}

int TYndKeyText::Cmp(const char* p) const
{
    return TRequestContext::CompareKeyWithLemma(Start, StartLen, Tail, p);
}

const char* TYndKeyText::UnpackKeyText(const char* p)
{
    Y_ASSERT(*p);
    Y_ASSERT(strlen(p) < MAXKEY_BUF); // historically; it can be MAXKEY_BUF
    unsigned hex = (unsigned char)*p++;
    if (1 <= hex && hex <= 31) {
        if (hex == 1)
            hex = (unsigned char)*p++;
        if (hex > (unsigned)StartLen) {
            if (Tail) {
                char *dst = Start + StartLen;
                const char *src = Tail;
                const size_t length = hex - StartLen;
                memcpy(dst, src, length);
            }
        }
        StartLen = hex;
        Start[hex] = 0;
    } else {
        StartLen = 0;
        --p;
    }
    Tail = p;
    while (*p++) {
    }
    return p;
}

const char* TYndKeyText::UnpackKey(YxRecord &rec, const char *data)
{
    data = UnpackKeyText(data);
    data = UnpackFullKey(rec, data);
    return data;
}

enum {
    MIN_WORD_SIZE = 5, //3+1+1 /* {diff-len}{key-char}{\0}{length}{count}
};

static bool SetRequestContext(const TFastAccessTable& fat, const TFileMap& keyFile, int block, TRequestContext& rc)
{
    if (rc.GetBlockNumber() == block)
        return true;

    const i64 nKeyFilePos = fat.GetKeyOffset(block);
    TFileMap localMap(keyFile);

    i64 mappedSize = fat.GetKeyBlockSize() * 2;
    bool overFlow = false;

    if (nKeyFilePos + mappedSize > localMap.Length()) {
        mappedSize = localMap.Length() - nKeyFilePos;
        overFlow = true;
    }

    try {
        localMap.Map(nKeyFilePos, (size_t)mappedSize);
    } catch (...) {
        Cerr << "SetRequestContext: " << CurrentExceptionMessage() << " Block: " << block << Endl;
        return false; // exit(1);
    }
    Y_ASSERT(localMap.Ptr() != nullptr);
    Y_ASSERT(localMap.MappedSize() == (size_t)mappedSize);

    int nFirstKey = fat.FirstKeyInBlock(block);
    int nEntriesOfBlock = fat.FirstKeyInBlock(block+1) - nFirstKey;

    if (overFlow) {
        TTempBuf hold(KeyBlockSize() * 2);
        memset(hold.Data(), 0, hold.Size());
        memcpy(hold.Data(), (char*)localMap.Ptr(), (int)mappedSize);
        rc.SetBlock(hold.Data(), (int)mappedSize, block, nFirstKey, nEntriesOfBlock, fat.GetOffset(block), fat.Compressed());
    } else {
        rc.SetBlock((char*)localMap.Ptr(), (int)mappedSize, block, nFirstKey, nEntriesOfBlock, fat.GetOffset(block), fat.Compressed());
    }

    localMap.Unmap();
    return true;
}

void TFastAccessTable::GetBlock(i32 number, i32 &block) const
{
    if (number < 0 || KeyCount() <= (ui32)number) {
        block = number < 0 || Infos.empty()? 0 : Infos.size() - 1;
        return;
    }
    if (block == UNKNOWN_BLOCK) {
        TVector<TKeyBlockInfo>::const_iterator I =
            LowerBound(Infos.begin(), Infos.end(), number, NIndexerCore::TKeyCountLess());
        Y_ASSERT(I != Infos.end());
        block = ui32(I - Infos.begin());
    }
    if (block < (i32)Infos.size() && (ui32)number >= Infos[block].KeyCount)
        block++;
}

i32 TFastAccessTable::BlockByWord(const char *key) const
{
    if (Infos.empty())
        return UNKNOWN_BLOCK;
    unsigned char c = (unsigned char)*key;
    TVector<TKeyBlockInfo>::const_iterator beg = Infos.begin() + FirstBlocks[c];
    TVector<TKeyBlockInfo>::const_iterator end = c == 255 ? Infos.end() : Infos.begin() + FirstBlocks[c + 1] + 1;

    TVector<TKeyBlockInfo>::const_iterator f = LowerBound(beg, end, key, NIndexerCore::TFirstKeyLess());
    i32 block = i32(f - Infos.begin());
    if (f == end || (block && strcmp(key, Infos[block].FirstKey) != 0))
        block--;
    return block;
}

TIndexInfo TFastAccessTable::Reset(IYndexStorage::FORMAT format, ui32 version, bool hasSubIndex) {
    BlockSize = KeyBlockSize(version);
    TIndexInfo indexInfo;
    indexInfo.SubIndexInfo = InitSubIndexInfo(format, version, hasSubIndex);
    indexInfo.Version = version;
    return indexInfo;
}

TIndexInfo TFastAccessTable::Open(NIndexerCore::TInputIndexFile& indexFile) {
    FAT = indexFile.ReadFastAccessTable(Infos, indexFile.GetVersion(), IsCompressed, indexFile.CreateInvMapping(), FirstBlocks);
    return Reset(indexFile.GetFormat(), indexFile.GetVersion(), indexFile.GetNumberOfBlocks() < 0);
}

TIndexInfo TFastAccessTable::Open(TMemoryMapStream& invStream, ui32 version, const TInvKeyInfo& invKeyInfo) {
    FAT = ReadFastAccessTable(invStream, version, IsCompressed, Infos, invStream.GetMapping(), FirstBlocks);
    return Reset(IYndexStorage::FINAL_FORMAT, version, invKeyInfo.NumberOfBlocks < 0);
}

TIndexInfo TFastAccessTable::Open(const TBlob& inv, ui32 version, const NIndexerCore::TInvKeyInfo& invKeyInfo) {
    FAT = ReadFastAccessTable(inv, version, IsCompressed, Infos, FirstBlocks);
    return Reset(IYndexStorage::FINAL_FORMAT, version, invKeyInfo.NumberOfBlocks < 0);
}

i32 FatLowerBound(const TFastAccessTable& fat, const TIndexInfo& indexInfo, const TFileMap& keyFile, const char* key, TRequestContext &rc)
{
    if (*key == 0)
        return 0; // empty key should be found

    i32 block = fat.BlockByWord(key);
    if (block == UNKNOWN_BLOCK || !SetRequestContext(fat, keyFile, block, rc))
        return -1;
    return rc.MoveToKey<TYndKeyText>(key, indexInfo);
}

const YxRecord*
GetEntryByNumber(const TFastAccessTable& fat, const TIndexInfo& indexInfo, const TFileMap& keyFile, TRequestContext& rc, i32 number, i32 &block) {
    if (number < 0 || fat.KeyCount() <= (ui32)number)
        return nullptr;
    fat.GetBlock(number, block);
    if (!SetRequestContext(fat, keyFile, block, rc))
        return nullptr;
    return rc.MoveToNumber<TYndKeyText>(number, indexInfo);
}
