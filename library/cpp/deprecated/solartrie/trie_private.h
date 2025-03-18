#pragma once

#include "solartrie.h"
#include "trie_conf.h"

#include <library/cpp/deprecated/solartrie/indexed_region/idxr_misc.h>

#include <util/generic/array_ref.h>
#include <util/system/yassert.h>
#include <util/stream/mem.h>

namespace NSolarTrie {
    enum ERefType {
        RT_INVALID = 0,
        RT_BUCKET,
        RT_FORK,
        RT_VALUE
    };

    const ui64 INVALID_VALUE = -1;

    struct TRef {
        static const ui64 BadOffset = (ui64(1) << 54) - 1;

        ui64 Char : 8;
        ui64 RefType : 2;
        ui64 Offset : 54;

        TRef(char ch = 0, ui64 off = -1)
            : Char((ui8)ch)
            , RefType(RT_INVALID)
            , Offset(off)
        {
        }

        operator bool() const {
            return !IsInvalid();
        }

        bool IsBucket() const {
            return RefType == RT_BUCKET;
        }

        bool IsFork() const {
            return RefType == RT_FORK;
        }

        bool IsValue() const {
            return RefType == RT_VALUE;
        }

        bool IsRoot() const {
            return IsFork() && !Offset;
        }

        bool IsInvalid() const {
            return RefType == RT_INVALID || Offset == BadOffset;
        }

        friend bool operator<(const TRef& a, const TRef& b) {
            return a.Char < b.Char;
        }

        friend bool operator==(const TRef& a, const TRef& b) {
            return a.Char == b.Char;
        }

        friend bool operator!=(const TRef& a, const TRef& b) {
            return !(a == b);
        }

        static bool EqualTargets(const TRef& a, const TRef& b) {
            return a.RefType == b.RefType && a.Offset == b.Offset;
        }

        static TRef NewBucket(ui8 c = 0, ui64 off = -1) {
            TRef ref(c, off);
            ref.RefType = RT_BUCKET;
            return ref;
        }

        static TRef NewFork(ui8 c = 0, ui64 off = -1) {
            TRef ref(c, off);
            ref.RefType = RT_FORK;
            return ref;
        }

        static TRef NewValue(ui64 off = -1) {
            TRef ref(0, off);
            ref.RefType = RT_VALUE;
            return ref;
        }
    };

    using TRefs = TVector<TRef>;

    /*
 * The layout is:
 * ForksStart           - VARINT8
 * BucketsStart         - VARINT8
 * ValuesStart + 1      - VARINT8
 * ForksIndexLen        - VARINT8
 * ForksIndex           - ui8 * (ForksIndexLen + (AddedLengths & 0x0F))
 * BucketsIndex         - ui16 * (BucketsIndexLen + (AddedLengths & 0xF0))
 *
 * Each entry in the BucketsIndex is ui8(minchar) | (ui16(maxchar) << 8),
 * minchar and maxchar are both inclusive.
 * This ensures that LowerBound(buckets.Begin(), buckets.End(), ui16(char) << 8)
 * gives the bucket with maxchar >= v.
 *
 * One has only to check if v >= minchar.
 */
    struct TCompactFork {
#pragma pack(push, 1)
        struct TUnalignedUI16 {
            ui16 Buf;
            TUnalignedUI16(unsigned b) {
                *this = b;
            }
            TUnalignedUI16& operator=(unsigned b) {
                memcpy(&Buf, &b, sizeof(Buf));
                return *this;
            }
            bool operator<(TUnalignedUI16 o) const {
                return Buf < o.Buf;
            }
        };
#pragma pack(pop)

        using TForksIndex = TArrayRef<const ui8>;
        using TBucketsIndex = TArrayRef<const TUnalignedUI16>;

        ui64 ForksStart = -1;
        ui64 BucketsStart = -1;
        ui64 ValueStart = -1;

        TForksIndex ForksIndex;
        TBucketsIndex BucketsIndex;

        TCompactFork() = default;

        TCompactFork(TStringBuf r) {
            Decode(r);
        }

        void Decode(TStringBuf r) {
            NBitIO::TBitInput bin(r);

            bool ok = true;

            ok &= bin.ReadWords<7>(ForksStart);
            ok &= bin.ReadWords<7>(BucketsStart);
            ok &= bin.ReadWords<7>(ValueStart); // 0 if empty

            --ValueStart;

            ui64 forksIndexLen = 0;
            ok &= bin.ReadWords<7>(forksIndexLen); // forks block size

            r.Skip(bin.GetOffset());

            ok &= forksIndexLen <= r.size();

            TStringBuf forks = r.SubStr(0, forksIndexLen);
            ForksIndex = TForksIndex{(const ui8*)forks.begin(), (const ui8*)forks.end()}; // forks block
            r.Skip(forksIndexLen);
            BucketsIndex = TBucketsIndex{(const TUnalignedUI16*)r.begin(), (const TUnalignedUI16*)r.end()}; // the rest is buckets block

            Y_ENSURE_EX(ok, TTrieException());
        }

        TRef Ref(ui8 v) const {
            if (!ForksIndex.empty()) {
                const ui8* f = LowerBound(ForksIndex.begin(), ForksIndex.end(), v);

                if (f != ForksIndex.end() && *f == v) {
                    return TRef::NewFork(v, ForksStart + f - ForksIndex.begin());
                }
            }

            if (!BucketsIndex.empty()) {
                TUnalignedUI16 vv = v << 8;
                const TUnalignedUI16* b = LowerBound(BucketsIndex.begin(), BucketsIndex.end(), vv);

                if (b != BucketsIndex.end() && ui32(b->Buf & 0x00FF) <= (ui32)v) {
                    return TRef::NewBucket(v, BucketsStart + b - BucketsIndex.begin());
                }
            }
            return TRef();
        }

        TRef FirstFork() const {
            return ForksIndex.empty() ? TRef() : TRef::NewFork(ForksIndex[0], ForksStart);
        }

        TRef FirstBucket() const {
            return BucketsIndex.empty() ? TRef() : TRef::NewBucket(0, BucketsStart);
        }

        TRef SelfValue() const {
            return HasValue() ? TRef::NewValue(ValueStart) : TRef();
        }

        TRef Next(TRef r) const {
            if (r.IsInvalid()) {
                r = SelfValue();
                if (!r) {
                    r = FirstBucket();
                }
                if (!r) {
                    r = FirstFork();
                }
            } else if (r.IsValue()) {
                r = FirstBucket();
                if (!r) {
                    r = FirstFork();
                }
            } else if (r.IsBucket()) {
                Y_ASSERT(r.Offset >= BucketsStart && r.Offset < BucketsStart + BucketsIndex.size());
                if (r.Offset + 1 < BucketsStart + BucketsIndex.size()) {
                    ++r.Offset;
                } else {
                    r = FirstFork();
                }
            } else if (r.IsFork()) {
                Y_ASSERT(r.Offset >= ForksStart && r.Offset < ForksStart + ForksIndex.size());
                if (r.Offset + 1 < ForksStart + ForksIndex.size()) {
                    r = TRef::NewFork(ForksIndex[r.Offset + 1 - ForksStart], r.Offset + 1);
                } else {
                    r = TRef();
                }
            }

            return r;
        }

        bool HasValue() const {
            return ValueStart != (ui64)-1;
        }
    };

    using TForkCursor = std::pair<TCompactFork, TRef>;
    using TTrieCursor = TVector<TForkCursor>;
    using TCompactForks = TVector<TCompactFork>;

    /*
 * The layout is:
 * SuffixesBlockLen     - VARINT8
 * Suffixes             - TSmallIndexedRegion
 * Values               - TSmallIndexedRegion
 */
    struct TCompactBucket {
        TStringBuf RawValues;

        NIndexedRegion::TAdaptiveSmallRegion Suffixes;
        NIndexedRegion::TAdaptiveSmallRegion Values;

        NCodecs::TCodecPtr SuffixCodec;

        bool FilledValues = false;
        bool Initialized = false;

        TCompactBucket() = default;

        void CopyTo(TCompactBucket& tgt) {
            tgt.RawValues = RawValues;
            tgt.FilledValues = FilledValues;
            tgt.Initialized = Initialized;

            Suffixes.CopyTo(tgt.Suffixes);

            if (FilledValues) {
                Values.CopyTo(tgt.Values);
            } else {
                Values.CopyCodecsTo(tgt.Values);
            }
        }

        void Init(const TSolarTrieConf& conf, bool sizeonly = false) {
            SuffixCodec = conf.SuffixCodec;
            FilledValues = false;

            Suffixes.Init(conf.SuffixLengthCodec, sizeonly ? nullptr : conf.SuffixBlockCodec);

            if (sizeonly)
                return;

            Values.Init(conf.ValueLengthCodec, conf.ValueBlockCodec);
            Initialized = true;
        }

        void Reset() {
            Suffixes.Clear();
            Values.Clear();
            RawValues.Clear();
            FilledValues = false;
        }

        void Clear() {
            Reset();
            Initialized = false;
        }

        size_t Size() const {
            return Suffixes.Size();
        }

        void Decode(TStringBuf r) {
            Reset();
            ui64 suffixesblocklen = 0;
            NBitIO::TBitInput bin(r);

            bool ok = true;

            ok &= bin.ReadWords<7>(suffixesblocklen);

            r.Skip(bin.GetOffset());

            ok &= suffixesblocklen <= r.size();

            Suffixes.Decode(r.SubStr(0, suffixesblocklen));
            RawValues = r.SubStr(suffixesblocklen, r.size()); // unpack only when needed

            Y_ENSURE_EX(ok, TTrieException());
        }

        void InitValues() {
            if (FilledValues) {
                return;
            }

            Values.Decode(RawValues);
            if (Y_UNLIKELY(Suffixes.Size() != Values.Size()))
                ythrow TTrieException();
            FilledValues = true;
        }

        TStringBuf GetValue(ui64 off) const {
            Y_ENSURE_EX(FilledValues, TTrieException());
            return Values.Get(off);
        }

        TStringBuf GetSuffix(ui64 off) const {
            return Suffixes.Get(off);
        }

        NIndexedRegion::TAdaptiveSmallRegion::TConstIterator Locate(TStringBuf& suff, TBuffer& val) const {
            if (!!SuffixCodec) {
                val.Clear();
                SuffixCodec->Encode(suff, val);
                suff = TStringBuf(val.Begin(), val.End());
            }

            return Suffixes.LowerBound(suff);
        }
    };

    struct TSolarTrie::TConstIterator::TImpl : TAtomicRefCount<TSolarTrie::TConstIterator::TImpl> {
        using TSelf = TSolarTrie::TConstIterator::TImpl;

        // Init
        const TSolarTrie::TImpl* Trie = nullptr;

        // Next
        TTrieCursor TrieCursor;
        mutable TCompactBucket CurrentBucket;
        mutable NIndexedRegion::TAdaptiveSmallRegion CurrentValues;
        size_t BucketCursor = 0;
        bool Exausted = false;

        mutable TBuffer TmpBuffer;
        mutable TBuffer KeyBuffer;
        mutable TBuffer ValBuffer;

        mutable bool KeyBufferValid = false;
        mutable bool ValBufferValid = false;

        TImpl() = default;

        void Init(const TSolarTrie::TImpl* t);

        void PrepareBucket(ui32 offset);
        bool Next();

        bool CurrentKey(TBuffer& key) const;
        bool CurrentVal(TBuffer& val) const;

        bool Current(TBuffer& key, TBuffer& val) const {
            return CurrentKey(key) && CurrentVal(val);
        }

        TStringBuf CurrentKey() const {
            if (KeyBufferValid) {
                return TStringBuf(KeyBuffer.Data(), KeyBuffer.Size());
            }

            KeyBuffer.Clear();

            if (!CurrentKey(KeyBuffer)) {
                return TStringBuf();
            }

            KeyBufferValid = true;
            return TStringBuf(KeyBuffer.Data(), KeyBuffer.Size());
        }

        TStringBuf CurrentVal() const {
            if (ValBufferValid) {
                return TStringBuf(ValBuffer.Data(), ValBuffer.Size());
            }

            ValBuffer.Clear();

            if (!CurrentVal(ValBuffer)) {
                return TStringBuf();
            }

            ValBufferValid = true;
            return TStringBuf(ValBuffer.Data(), ValBuffer.Size());
        }

        bool HasCurrent() const {
            return !Exausted && !TrieCursor.empty();
        }

        void Restart() {
            Init(Trie);
        }

        void CopyTo(TImpl& other) const {
            other.Trie = Trie;

            other.TrieCursor = TrieCursor;
            CurrentBucket.CopyTo(other.CurrentBucket);
            CurrentValues.CopyTo(other.CurrentValues);
            other.BucketCursor = BucketCursor;
            other.Exausted = Exausted;

            other.TmpBuffer = TmpBuffer;
            other.KeyBuffer = KeyBuffer;
            other.ValBuffer = ValBuffer;

            other.KeyBufferValid = KeyBufferValid;
            other.ValBufferValid = ValBufferValid;
        }
    };

    /*
 * The layout is:
 * Configuration (magic, version, codecs)
 *                      - TTrieConf
 * ForksBlockLen        - ui64
 * ForkValuesBlockLen   - ui64
 * CompactForks         - TBigIndexedRegion, blocks are TCompactFork
 * ForkValues           - TBigIndexedRegion, blocks are TSmallIndexedRegion of Conf.ForkValueBlockSize values
 * Buckets              - TBigIndexedRegion, blocks are TCompactBucket
 */

    class TSolarTrie::TImpl: public TAtomicRefCount<TImpl> {
    public:
        TSolarTrieConf Conf;

        TBlob Memory;

        NIndexedRegion::TBigIndexedRegion CompactForks;
        NIndexedRegion::TBigIndexedRegion ForkValues;
        NIndexedRegion::TBigIndexedRegion Buckets;

        using TCompactBucketTlsCache = NCodecs::TTlsCache<TCompactBucket>;
        using TSmallRegion = NIndexedRegion::TAdaptiveSmallRegion;
        using TSmallRegionTlsCache = NCodecs::TTlsCache<TSmallRegion>;

    public:
        void Wrap(TBlob b) {
            Memory = b;

            {
                TMemoryInput min(b.Data(), b.Size());
                Conf.Load(&min);
                b = b.SubBlob(b.Size() - min.Avail(), b.Size());
            }

            DecodeRegion(b, CompactForks);
            DecodeRegion(b, ForkValues);
            DecodeRegion(b, Buckets);
        }

        bool Get(TStringBuf key, TBuffer& val) const {
            auto tmpBuffer = NCodecs::TBufferTlsCache::TlsInstance().Item();
            return Get(key, val, tmpBuffer.Get());
        }

        bool Get(TStringBuf key, TBuffer& val, TBuffer& auxbuf) const {
            EncodeKey(key, auxbuf);
            TRef r = LocateBucket<false>(key, nullptr);
            return LocateValue(r, key, val);
        }

        const NIndexedRegion::TBigIndexedRegion& GetForks() const {
            return CompactForks;
        }

        const NIndexedRegion::TBigIndexedRegion& GetForkValues() const {
            return ForkValues;
        }

        const NIndexedRegion::TBigIndexedRegion& GetBuckets() const {
            return Buckets;
        }

        const TSolarTrieConf& GetConf() const {
            return Conf;
        }

        size_t Size() const {
            size_t res = 0;
            {
                const size_t sz = Buckets.Size();
                for (size_t i = 0; i < sz; ++i) {
                    auto tmpBucketSz = TCompactBucketTlsCache::TlsInstance().Item();
                    TCompactBucket& b = tmpBucketSz.Get();

                    if (!b.Initialized) {
                        b.Init(Conf, true);
                    }

                    b.Decode(Buckets[i]);
                    res += b.Size();
                }
            }

            if (!ForkValues.Empty()) {
                size_t s = (ForkValues.Size() - 1) * Conf.ForkValuesBlockSize;
                res += s;
                auto tmpForkValuesSz = TSmallRegionTlsCache::TlsInstance().Item();
                TSmallRegion& reg = tmpForkValuesSz.Get();
                reg.Init(Conf.ValueLengthCodec);
                reg.Decode(ForkValues[ForkValues.Size() - 1]);
                res += reg.Size();
            }

            return res;
        }

        bool Empty() const {
            if (CompactForks.Empty()) {
                return true;
            }

            TCompactFork f(CompactForks[0]);
            return f.ForksIndex.empty() && f.BucketsIndex.empty() && !f.HasValue();
        }

        TSolarTrie::TConstIterator Iterator() const {
            TSolarTrie::TConstIterator it;
            it.Impl->Init(this);
            return it;
        }

        Y_FORCE_INLINE void PushFork(TTrieCursor& c, ui64 off) const {
            TCompactFork f;
            f.Decode(GetForks()[off]);
            c.push_back(std::make_pair(f, TRef()));
        }

        void Dump(IOutputStream&) const;

        ui8 EncodeKey(TStringBuf& key, TBuffer& buff) const {
            if (!!Conf.KeyCodec) {
                buff.Clear();
                ui8 tail = Conf.KeyCodec->Encode(key, buff);
                key = TStringBuf(buff.Begin(), buff.End());
                return tail;
            }

            return 0;
        }

        template <bool buildcursor>
        TRef LocateBucket(TStringBuf& key, TTrieCursor* cursor) const {
            TRef current = TRef::NewFork(0, 0);
            TCompactFork parent;

            while (!key.empty() && current.IsFork()) {
                parent.Decode(CompactForks[current.Offset]);
                current = parent.Ref(key[0]);

                if (buildcursor) {
                    cursor->push_back(std::make_pair(parent, current));
                }

                if (current.IsFork()) {
                    key.Skip(1);
                }
            }

            if (current.IsFork()) {
                Y_VERIFY(key.empty(), "shit!");
                parent.Decode(CompactForks[current.Offset]);
                current = TRef::NewValue(parent.ValueStart);

                if (buildcursor) {
                    cursor->push_back(std::make_pair(parent, current));
                }

                return current;
            }

            return current;
        }

        bool LocateValue(TRef ref, TStringBuf suff, TBuffer& val) const {
            using namespace NCodecs;
            if (ref.IsInvalid()) {
                return false;
            }

            TStringBuf r;

            if (ref.IsValue()) {
                auto tmpForkValues = TSmallRegionTlsCache::TlsInstance().Item();
                TSmallRegion& reg = tmpForkValues.Get();

                reg.Init(Conf.ValueLengthCodec, Conf.ValueBlockCodec);
                reg.Decode(ForkValues[ref.Offset / Conf.ForkValuesBlockSize]);
                r = reg.Get(ref.Offset % Conf.ForkValuesBlockSize);
            } else if (ref.IsBucket()) {
                auto tmpBucket = TCompactBucketTlsCache::TlsInstance().Item();
                TCompactBucket& buck = tmpBucket.Get();

                if (!buck.Initialized) {
                    buck.Init(Conf);
                }

                buck.Decode(Buckets[ref.Offset]);

                NIndexedRegion::TAdaptiveSmallRegion::TConstIterator res = buck.Locate(suff, val);

                if (res.Offset >= buck.Size() || *res != suff) {
                    return false;
                }

                buck.InitValues();
                r = buck.GetValue(res.Offset);
            } else {
                ythrow TTrieException();
            }

            if (Conf.ValueCodec.Get()) {
                val.Clear();
                Conf.ValueCodec->Decode(r, val);
            } else {
                val.Assign(r.data(), r.size());
            }

            return true;
        }

        static void DecodeRegion(TBlob& b, NIndexedRegion::TBigIndexedRegion& r) {
            ui64 len = 0;

            {
                TMemoryInput min(b.Data(), b.Size());
                ::Load(&min, len);
                b = b.SubBlob(b.Size() - min.Avail(), b.Size());
            }

            Y_ENSURE_EX(len <= b.Size(), TTrieException());

            const auto& sb = b.SubBlob(len);
            r.Decode(TStringBuf(sb.AsCharPtr(), sb.Size()));
            b = b.SubBlob(len, b.Size());
        }
    };

}
