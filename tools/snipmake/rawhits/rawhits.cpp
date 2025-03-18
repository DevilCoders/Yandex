#include "rawhits.h"

#include <ysite/yandex/posfilter/dochitsbuf.h>
#include <ysite/yandex/posfilter/hits_loader.h>
#include <ysite/yandex/posfilter/tr_iterator.h>
#include <kernel/qtree/richrequest/richnode.h>

#include <kernel/keyinv/indexfile/fat.h>
#include <kernel/keyinv/indexfile/memoryportion.h>

#include <util/generic/ptr.h>

typedef NIndexerCore::NIndexerDetail::TInputMemoryStream    TInMemStream;

//TODO: merge with yweb/serpapi/core/base

class TInMemKeysAndPositions : private TNonCopyable, public IKeysAndPositions {
private:
    TBlob Inv;
    TBlob Key;

    TFastAccessTable Fat;
    TIndexInfo IndexInfo;
public:
    void GetBlob(TBlob& data, i64 offset, ui32 length, READ_HITS_TYPE type) const override;
    const YxRecord* EntryByNumber(TRequestContext &rc, i32 number, i32 &block) const override; // for sequential access (avoid LowerBound())
    i32 LowerBound(const char *word, TRequestContext &rc) const override; // for group searches [...)
    const TIndexInfo& GetIndexInfo() const override;

    void InitSearch(const TString& rawKeyData, const TString& rawInvData);

    ui32 KeyCount() const override;
    const char *WordByNumber(TRequestContext &rc, i32 number) const;
};

const TIndexInfo& TInMemKeysAndPositions::GetIndexInfo() const {
    return IndexInfo;
}

static bool SetRequestContext(const TFastAccessTable& fat, const TBlob& key, int block, TRequestContext& rc) {
    if (rc.GetBlockNumber() == block)
        return true;

    const i64 nKeyFilePos = fat.GetKeyOffset(block);

    i64 mappedSize = fat.GetKeyBlockSize() * 2;
    bool overflow = false;

    if (nKeyFilePos + mappedSize > (i64) key.Length()) {
        mappedSize = (i64) (key.Length() - nKeyFilePos);
        overflow = true;
    }

    int nFirstKey = fat.FirstKeyInBlock(block);
    int nEntriesOfBlock = fat.FirstKeyInBlock(block+1) - nFirstKey;

    const char* pos = key.AsCharPtr() + nKeyFilePos;
    if (overflow) {
        TTempBuf hold(NIndexerCore::KeyBlockSize() * 2);
        memset(hold.Data() + mappedSize, 0, hold.Size() - mappedSize);
        memcpy(hold.Data(), pos, (int)mappedSize);
        rc.SetBlock(hold.Data(), (int)mappedSize, block, nFirstKey, nEntriesOfBlock, fat.GetOffset(block), fat.Compressed());
    } else {
        rc.SetBlock(pos, (int) mappedSize, block, nFirstKey, nEntriesOfBlock, fat.GetOffset(block), fat.Compressed());
    }

    return true;
}

i32 FatLowerBound(const TFastAccessTable& fat, const TIndexInfo& info, const TBlob& key, const char *word, TRequestContext &rc) {
    if (*word == 0)
        return 0; // empty key should be found

    i32 block = fat.BlockByWord(word);
    if (block == UNKNOWN_BLOCK || !SetRequestContext(fat, key, block, rc))
        return -1;
    return rc.MoveToKey<TYndKeyText>(word, info);
}

const YxRecord* GetEntryByNumber(const TFastAccessTable& fat, const TIndexInfo& indexInfo, const TBlob& key, TRequestContext& rc, i32 number, i32 &block) {
    if (number < 0 || fat.KeyCount() <= (ui32)number)
        return nullptr;
    fat.GetBlock(number, block);
    if (!SetRequestContext(fat, key, block, rc))
        return nullptr;
    return rc.MoveToNumber<TYndKeyText>(number, indexInfo);
}

void TInMemKeysAndPositions::GetBlob(TBlob& data, i64 offset, ui32 length, READ_HITS_TYPE type) const {
    switch (type) {
        case RH_DEFAULT:
        case RH_FORCE_MAP:
            data = Inv.SubBlob(offset, offset + length);
        break;
        case RH_FORCE_ALLOC:
            data = Inv.SubBlob(offset, offset + length).DeepCopy();
        break;
    }
}

const YxRecord* TInMemKeysAndPositions::EntryByNumber(TRequestContext &rc, i32 number, i32 & block) const {
    return GetEntryByNumber(Fat, IndexInfo, Key, rc, number, block);
}

i32 TInMemKeysAndPositions::LowerBound(const char *word, TRequestContext &rc) const {
    return FatLowerBound(Fat, IndexInfo, Key, word, rc);
}

void TInMemKeysAndPositions::InitSearch(const TString& rawKeyData, const TString& rawInvData) {
    Y_ASSERT(!rawKeyData.empty());
    Y_ASSERT(!rawInvData.empty());
    Fat.Clear();
    Inv = TBlob::FromString(rawInvData);
    Key = TBlob::FromString(rawKeyData);

    ui32 version = 0;
    TInMemStream invStream(rawInvData.data(), rawInvData.size());
    NIndexerCore::TInvKeyInfo invKeyInfo;
    ReadIndexInfoFromStream(invStream, version, invKeyInfo);

    IndexInfo = Fat.Open(Inv, version, invKeyInfo);

    const char* DEL = "\x7f";
    TRequestContext rc;
    i32 num = LowerBound(DEL, rc);
    i32 block = UNKNOWN_BLOCK;
    const YxRecord *record = EntryByNumber(rc, num, block);
    IndexInfo.HasUtfKeys = record && record->TextPointer[0] == DEL[0];
}

ui32 TInMemKeysAndPositions::KeyCount() const {
    return Fat.KeyCount();
}

const char *TInMemKeysAndPositions::WordByNumber(TRequestContext &rc, i32 number) const {
    i32 block = UNKNOWN_BLOCK;
    const YxRecord* entry = EntryByNumber(rc, number, block);
    if (!entry)
        return "???";
    return entry->TextPointer;
}

struct THitsBufPool
{
    enum {
        N_SIZE = 48000,
        N_JUNK = 15000
    };

    TFullPositionEx PosBuf[N_SIZE];
    THitInfo HitInfoBuf[N_SIZE];
    int Idx;

    THitsBufPool()
        : Idx(0)
    {
    }

    void Reset() {
        Idx = 0;
    }

    int GetFreeSpaceSize() const {
        return N_SIZE - Idx;
    }
};

void LoadDocumentHits(ui32 docId, TDocumentHitsBuf *hits, THitsBufPool *pool, ITRIterator *iter) {
    int space = pool->GetFreeSpaceSize();
    hits->Pos = pool->PosBuf + pool->Idx;
    hits->HitInfo = pool->HitInfoBuf + pool->Idx;
    int count = iter->GetAllDocumentPositions(docId, hits->Pos, space, hits->WordMask);
    hits->Count = count;
    iter->GetLastDocumentFlags(docId, &hits->Flags);
    pool->Idx += hits->Count;
}

//TODO: verify/compare with what basesearch does

void GetSnippetHits(TVector<ui32>& sents, TVector<ui32>& masks, const TString& skeyWithFat, const TString& sinvWithFat, const TRichTreeConstPtr& Richtree) {
    TInMemKeysAndPositions kp;
    THitsLoader hl(kp);
    kp.InitSearch(skeyWithFat, sinvWithFat);
    TAutoPtr<ITRIterator> i(CreateTRIteratorSimple(&hl, &hl, *Richtree->Root.Get()));
    if (!i) {
        return;
    }
    TDocumentHitsBuf Hits;
    TNextDocSwitcher Switcher(i.Get(), nullptr);
    TNextDocSwitcher::TResult SwitchResult;
    THitsBufPool Pool;
    ui32 snt = 0;
    ui32 msk = 0;
    bool first = true;
    while (Switcher.Switch(&SwitchResult)) {
        Pool.Reset();
        LoadDocumentHits(SwitchResult.DocId, &Hits, &Pool, i.Get());
        for (int i = 0; i < Hits.Count; ++i) {
            const TFullPositionEx& p = Hits.Pos[i];
            const TWordPosition& wp = (const TWordPosition&)p.Pos.Beg;
            if (wp.InTitle()) {
                continue;
            }
            if (!first && wp.Break() != snt) {
                sents.push_back(snt);
                masks.push_back(msk);
                msk = 0;
            }
            first = false;
            snt = wp.Break();
            msk |= (1 << (Min<ui32>(p.WordIdx, 32)));
        }
        if (!first) {
            sents.push_back(snt);
            masks.push_back(msk);
        }
    }
}
