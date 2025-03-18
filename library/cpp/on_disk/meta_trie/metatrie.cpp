#include "index_digest.h"
#include "metatrie_impl.h"

#include "subtrie_compacttrie.h"
#include "subtrie_solartrie.h"
#include "subtrie_codectrie.h"

#include "simple_builder.h"

#include <util/string/vector.h>
#include <util/ysaveload.h>

namespace NMetatrie {
    enum EBlobType {
        BT_NONE = -1,
        BT_SUBTRIE = 0,
        BT_INDEX = 1,
        BT_COUNT = 2,
    };

    const char MAGIC[] = "METATRIE";
    const ui64 VERSION = 1;
    const ui32 MAGIC_SIZES = 8;
    const ui32 MAGIC_SSIZES = MAGIC_SIZES + 1;

    const char COMPTRIE_STR[] = "COMPTRIE";
    const char IDX_DGST_STR[] = "IDX_DGST";
    const char SOLAR_TR_STR[] = "SOLAR_TR";
    const char CODEC_TR_STR[] = "CODEC_TR";
    const char BT_SUBTR_STR[] = "BT_SUBTR";
    const char BT_INDEX_STR[] = "BT_INDEX";

    static_assert(sizeof(VERSION) == MAGIC_SIZES, "expect sizeof(VERSION) == MAGIC_SIZES");
    static_assert(sizeof(COMPTRIE_STR) == MAGIC_SSIZES, "expect sizeof(COMPTRIE_STR) == MAGIC_SSIZES");
    static_assert(sizeof(IDX_DGST_STR) == MAGIC_SSIZES, "expect sizeof(IDX_DGST_STR) == MAGIC_SSIZES");
    static_assert(sizeof(SOLAR_TR_STR) == MAGIC_SSIZES, "expect sizeof(SOLAR_TR_STR) == MAGIC_SSIZES");
    static_assert(sizeof(CODEC_TR_STR) == MAGIC_SSIZES, "expect sizeof(CODEC_TR_STR) == MAGIC_SSIZES");
    static_assert(sizeof(BT_SUBTR_STR) == MAGIC_SSIZES, "expect sizeof(BT_SUBTR_STR) == MAGIC_SSIZES");
    static_assert(sizeof(BT_INDEX_STR) == MAGIC_SSIZES, "expect sizeof(BT_INDEX_STR) == MAGIC_SSIZES");

    const char* const SUBTRIE_TYPE[] = {
        COMPTRIE_STR, SOLAR_TR_STR, CODEC_TR_STR};

    const char* const INDEX_TYPE[] = {
        IDX_DGST_STR};

    const char* const BLOB_TYPE[] = {
        BT_SUBTR_STR, BT_INDEX_STR};

    union TMagic {
        char Buf[8];
        ui64 Num;

        TMagic()
            : Num()
        {
        }

        bool Load(IInputStream* in) {
            Num = 0;
            return in->Load(Buf, MAGIC_SIZES) == MAGIC_SIZES;
        }

        bool Assert(IInputStream* in, const char* m) {
            return Load(in) && !memcmp(Buf, m, MAGIC_SIZES);
        }

        ui64 ReadNum(IInputStream* in) {
            return Load(in) ? Num : -1;
        }

        int ReadType(IInputStream* in, const char* const m[], int max) {
            if (!Load(in))
                return -1;

            for (int i = 0; i < max; ++i) {
                if (!memcmp(Buf, m[i], MAGIC_SIZES))
                    return i;
            }

            return -1;
        }
    };

    TKey::TKey() {
    }
    TKey::TKey(const TKey& v) {
        *this = v;
    }
    TKey::~TKey() {
    }

    TKeyImpl* TKey::Inner() {
        if (!Impl)
            Impl = new TKeyImpl;
        return Impl.Get();
    }

    TKey& TKey::operator=(const TKey& v) {
        Impl = v.Impl;
        return *this;
    }

    TStringBuf TKey::Get() const {
        return !Impl ? TStringBuf() : Impl->Key;
    }

    TVal::TVal() {
    }
    TVal::TVal(const TVal& v) {
        *this = v;
    }
    TVal::~TVal() {
    }

    TValImpl* TVal::Inner() {
        if (!Impl)
            Impl = new TValImpl;
        return Impl.Get();
    }

    TVal& TVal::operator=(const TVal& v) {
        Impl = v.Impl;
        return *this;
    }

    TStringBuf TVal::Get() const {
        return !Impl ? TStringBuf() : Impl->Val;
    }

    struct TBlobInfo {
        TBlob Blob;
        EBlobType BlobType;
        ESubtrieType SubtrieType;
        EIndexType IndexType;

        TBlobInfo()
            : BlobType(BT_NONE)
            , SubtrieType(ST_NONE)
            , IndexType(IT_NONE)
        {
        }

        bool Read(TBlob& b) {
            TMemoryInput min(b.Data(), b.Size());
            TMagic magic;

            if ((BlobType = (EBlobType)magic.ReadType(&min, BLOB_TYPE, Y_ARRAY_SIZE(BLOB_TYPE))) < 0)
                return false;

            switch (BlobType) {
                case BT_SUBTRIE:
                    if ((SubtrieType = (ESubtrieType)magic.ReadType(&min, SUBTRIE_TYPE, Y_ARRAY_SIZE(SUBTRIE_TYPE))) < 0)
                        ythrow TMetatrieException();
                    break;
                case BT_INDEX:
                    if ((IndexType = (EIndexType)magic.ReadType(&min, INDEX_TYPE, Y_ARRAY_SIZE(INDEX_TYPE))) < 0)
                        ythrow TMetatrieException();
                    break;
                default:
                    ythrow TMetatrieException();
            }

            ui64 len = magic.ReadNum(&min);

            if (len > min.Avail())
                ythrow TMetatrieException();

            size_t off = b.Size() - min.Avail();
            Blob = b.SubBlob(off, len + off);
            b = b.SubBlob(len + off, b.Size());
            return true;
        }
    };

    static ITrie* NewTrie(const TBlobInfo& b) {
        switch (b.SubtrieType) {
            default:
                ythrow TMetatrieException();
                return nullptr;
            case ST_COMPTRIE:
                return new TCompactSubtrieWrapper(b.Blob);
            case ST_SOLARTRIE:
                return new TSolarSubtrieWrapper(b.Blob);
            case ST_CODECTRIE:
                return new TCodecSubtrieWrapper(b.Blob);
        }
    }

    static IIndex* NewIndex(const TBlobInfo& b) {
        switch (b.IndexType) {
            default:
                ythrow TMetatrieException();
                return nullptr;
            case IT_DIGEST:
                return new TDigestIndex(b.Blob);
        }
    }

    struct TIteratorImpl {
        TIntrusiveConstPtr<TMetatrieImpl> Trie;
        size_t CurrentSubtrie;
        TIntrusivePtr<IIterator> SubtrieIterator;

        TIteratorImpl(const TMetatrieImpl* t)
            : Trie((TMetatrieImpl*)t)
            , CurrentSubtrie()
        {
        }

        inline bool Next();
        inline bool CurrentKey(TKey&) const;
        inline bool CurrentVal(TVal&) const;
    };

    struct TMetatrieImpl : TAtomicRefCount<TMetatrieImpl> {
        typedef TIntrusivePtr<IIndex> TIndexPtr;
        typedef TIntrusivePtr<ITrie> TTriePtr;
        typedef TVector<TTriePtr> TTriePtrs;

        TBlob Blob;

        TIndexPtr Index;
        TTriePtrs Subtries;

        size_t IndexSize;
        size_t AverTrieSize[ST_COUNT];
        size_t TriesCount[ST_COUNT];
        size_t MinTrieSize[ST_COUNT];
        size_t MaxTrieSize[ST_COUNT];

        TString TrieReport[ST_COUNT];

        TMetatrieImpl(TBlob b)
            : Blob(b)
            , IndexSize()
        {
            Zero(AverTrieSize);
            Zero(TriesCount);
            memset(&MinTrieSize, 1, sizeof(MinTrieSize));
            Zero(MaxTrieSize);

            {
                TMemoryInput in(b.Data(), b.Size());
                TMagic magic;

                if (!magic.Assert(&in, MAGIC) || magic.ReadNum(&in) > VERSION)
                    ythrow TMetatrieException();

                b = b.SubBlob(b.Size() - in.Avail(), b.Size());
            }

            TBlobInfo bi;

            while (bi.Read(b)) {
                switch (bi.BlobType) {
                    default:
                        ythrow TMetatrieException();
                    case BT_SUBTRIE:
                        Subtries.push_back(NewTrie(bi));

                        AverTrieSize[bi.SubtrieType] += bi.Blob.Size();
                        MinTrieSize[bi.SubtrieType] = Min(MinTrieSize[bi.SubtrieType], bi.Blob.Size());
                        MaxTrieSize[bi.SubtrieType] = Max(MaxTrieSize[bi.SubtrieType], bi.Blob.Size());

                        if (!TriesCount[bi.SubtrieType])
                            TrieReport[bi.SubtrieType] = Subtries.back()->Report();

                        ++TriesCount[bi.SubtrieType];

                        break;
                    case BT_INDEX:
                        IndexSize = bi.Blob.Size();
                        Index = NewIndex(bi);
                        break;
                }
            }

            if (!Index)
                ythrow TMetatrieException();
        }

        bool Get(TStringBuf key, TVal& v) const {
            if (!Index)
                return false;

            ui64 off = Index->GetIndex(key);

            if (off >= Subtries.size())
                return false;

            return Subtries[off]->Get(key, v);
        }

        TMetatrie::TConstIterator Iterator() const {
            return TMetatrie::TConstIterator(new TIteratorImpl(this));
        }

        TString Report() const {
            TString res;

            for (int i = 0; i < ST_COUNT; ++i) {
                if (!TriesCount[i])
                    continue;

                res.append(Sprintf("Subtrie:%s{count:%lu, aversz:%.3f%s, minsz:%.3f%s, maxsz:%.3f%s}[%s], ", SUBTRIE_TYPE[i], TriesCount[i], Size(float(AverTrieSize[i]) / TriesCount[i]), Label(float(AverTrieSize[i]) / TriesCount[i]), Size(MinTrieSize[i]), Label(MinTrieSize[i]), Size(MaxTrieSize[i]), Label(MaxTrieSize[i]), TrieReport[i].data()));
            }

            res.append(Sprintf("Index:%s{sz:%.3f%s}[%s]", INDEX_TYPE[Index->GetType()], Size(IndexSize), Label(IndexSize), Index->Report().data()));
            return res;
        }

        static bool TooSmall(size_t sz) {
            return sz / float(1 << 20) < 0.1;
        }

        static float Size(size_t sz) {
            return TooSmall(sz) ? sz / float(1 << 10) : sz / float(1 << 20);
        }

        static const char* Label(size_t sz) {
            return TooSmall(sz) ? "kb" : "Mb";
        }
    };

    bool TIteratorImpl::Next() {
        if (!Trie)
            return false;

        while (true) {
            if (!SubtrieIterator) {
                if (CurrentSubtrie >= Trie->Subtries.size())
                    return false;
                SubtrieIterator = Trie->Subtries[CurrentSubtrie++]->Iterator();
            }

            if (SubtrieIterator->Next())
                return true;

            SubtrieIterator = nullptr;
        }

        return false;
    }

    bool TIteratorImpl::CurrentKey(TKey& k) const {
        if (!SubtrieIterator)
            return false;
        return SubtrieIterator->CurrentKey(k);
    }

    bool TIteratorImpl::CurrentVal(TVal& v) const {
        if (!SubtrieIterator)
            return false;
        return SubtrieIterator->CurrentVal(v);
    }

    TMetatrie::TConstIterator::TConstIterator() {
    }
    TMetatrie::TConstIterator::TConstIterator(TIteratorImpl* i)
        : Impl(i)
    {
    }
    TMetatrie::TConstIterator::TConstIterator(const TConstIterator& t) {
        *this = t;
    }
    TMetatrie::TConstIterator::~TConstIterator() {
    }
    TMetatrie::TConstIterator& TMetatrie::TConstIterator::operator=(const TConstIterator& t) {
        Impl = t.Impl;
        return *this;
    }

    bool TMetatrie::TConstIterator::Next() {
        return !Impl ? false : Impl->Next();
    }

    bool TMetatrie::TConstIterator::CurrentKey(TKey& k) const {
        return !Impl ? false : Impl->CurrentKey(k);
    }

    bool TMetatrie::TConstIterator::CurrentVal(TVal& v) const {
        return !Impl ? false : Impl->CurrentVal(v);
    }

    TMetatrie::TMetatrie() {
    }
    TMetatrie::TMetatrie(TBlob b) {
        Init(b);
    };
    TMetatrie::TMetatrie(const TMetatrie& t) {
        *this = t;
    }
    TMetatrie::~TMetatrie() {
    }
    TMetatrie& TMetatrie::operator=(const TMetatrie& t) {
        Impl = t.Impl;
        return *this;
    }

    void TMetatrie::Init(TBlob b) {
        Impl = new TMetatrieImpl(b);
    }

    bool TMetatrie::Get(TStringBuf key, TVal& v) const {
        return !Impl ? false : Impl->Get(key, v);
    }

    TString TMetatrie::Report() const {
        return !Impl ? "empty metatrie" : Impl->Report();
    }

    TBlob TMetatrie::Data() const {
        return !Impl ? TBlob() : Impl->Blob;
    }

    TMetatrie::TConstIterator TMetatrie::Iterator() const {
        return !Impl ? TConstIterator() : Impl->Iterator();
    }

    struct TMetatrieBuilder::TImpl {
        TIndexBuilderPtr Builder;
        IOutputStream& Out;
        TString LastKey;
        size_t Count;
        bool Committed;

        TImpl(IOutputStream& out, TIndexBuilderPtr b)
            : Builder(b)
            , Out(out)
            , Count()
            , Committed()
        {
            if (!Builder)
                Builder = new TDigestIndexBuilder(1);

            Out.Write(MAGIC, MAGIC_SIZES);
            ::Save(&Out, VERSION);
        }

        void AddSubtrie(TStringBuf firstkey, TStringBuf lastkey, ESubtrieType type, TStringBuf blob) {
            Builder->CheckRange(LastKey, firstkey);
            Builder->CheckRange(firstkey, lastkey);

            LastKey = lastkey;

            if (type < 0 || type >= ST_COUNT)
                ythrow TMetatrieException() << "Invalid ESubtrieType value " << (int)type;

            Out.Write(BLOB_TYPE[BT_SUBTRIE], MAGIC_SIZES);
            Out.Write(SUBTRIE_TYPE[type], MAGIC_SIZES);
            ::Save(&Out, (ui64)blob.size());
            Out.Write(blob.data(), blob.size());
            Builder->Add(firstkey, lastkey, Count++);
        }

        void Commit() {
            if (Committed)
                return;

            Out.Write(BLOB_TYPE[BT_INDEX], MAGIC_SIZES);
            Out.Write(INDEX_TYPE[Builder->GetType()], MAGIC_SIZES);

            {
                TBlob idx = Builder->Commit();
                ::Save(&Out, (ui64)idx.Size());
                Out.Write(idx.Data(), idx.Size());
            }

            Out.Flush();
            Committed = true;
        }
    };

    TMetatrieBuilder::TMetatrieBuilder() {
    }
    TMetatrieBuilder::TMetatrieBuilder(IOutputStream& out, TIndexBuilderPtr b) {
        Init(out, b);
    }

    TMetatrieBuilder::~TMetatrieBuilder() {
        CommitAll();
    }

    void TMetatrieBuilder::Init(IOutputStream& out, TIndexBuilderPtr b) {
        Impl.Reset(new TImpl(out, b));
    }

    void TMetatrieBuilder::AddSubtrie(TStringBuf firstkey, TStringBuf lastkey, ESubtrieType type, TStringBuf blob) {
        Impl->AddSubtrie(firstkey, lastkey, type, blob);
    }

    void TMetatrieBuilder::AddSubtrie(TSubtrieBuilderBase& t) {
        t.Commit();
        AddSubtrie(t.FirstKey, t.LastKey, t.GetType(), TStringBuf((const char*)t.Result.Data(), t.Result.Size()));
    }

    void TMetatrieBuilder::CommitAll() {
        Impl->Commit();
    }

    TIndexBuilderPtr CreateIndexBuilder(const TMetatrieConf& conf, ui64 nportions, ui64 hashedPrefixLength) {
        switch (conf.IndexType) {
            default:
                ythrow TMetatrieException() << "unsupported index type " << (int)conf.IndexType;
                return nullptr;
            case IT_DIGEST:
                return new TDigestIndexBuilder(GetPrimeBase(nportions), hashedPrefixLength);
        }
    };

    TSubtrieBuilderPtr CreateSubtrieBuilder(const TMetatrieConf& conf) {
        switch (conf.SubtrieType) {
            default:
                ythrow TMetatrieException() << "unsupported trie type " << (int)conf.SubtrieType;
                return nullptr;
            case ST_COMPTRIE:
                return new TCompactSubtrieBuilder(conf.CompTrieConf);
            case ST_SOLARTRIE:
                return new TSolarSubtrieBuilder(conf.SolarTrieConf);
            case ST_CODECTRIE:
                return new TCodecSubtrieBuilder(conf.CodecTrieConf);
        }
    }

}
