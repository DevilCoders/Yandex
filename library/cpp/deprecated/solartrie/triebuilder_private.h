#pragma once

#include "trie_private.h"
#include "triebuilder.h"

#include <library/cpp/containers/paged_vector/paged_vector.h>

#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/generic/bitops.h>

#include <util/memory/pool.h>
#include <util/system/defaults.h>
#include <util/system/rusage.h>

namespace NSolarTrie {
    /*
 * The structure is based upon the burst trie with some improvements after solar@.
 * (see original article here: http://www.csse.unimelb.edu.au/~jz/fulltext/acmtois02.pdf).
 *
 * The main difference against the classic burst trie is that the buckets are reused for adjustent suffixes
 * instead of forking. This saves the space otherwise wasted for bookkeeping.
 *
 * Example below illustrates the difference:
 *
 * F: [empty]
 *
 * quark -> Trie
 *
 * F:
 * .[q->B]:
 * .......quark
 *
 * bark, foo, bar, baz, quux, rex -> Trie
 *
 * F:
 * .[q->B]:
 * .......bar,bark,baz,foo,quark,quux,rex
 *
 * SPLIT!
 *
 * F:
 * .[b->B]:...............[f->B]:
 * .......bar,bark,baz..........foo,quark,quux,rex
 *
 * boom -> Trie
 *
 * F:
 * .[b->B]:....................[f->B]:
 * .......bar,bark,baz,boom..........foo,quark,quux,rex
 *
 * EXTRACT PREFIX!
 *
 * F:
 * .[b->F]:...........................[f->B]:
 * .......[a->B]:...........................foo,quark,quux,rex
 * .............ar,ark,az,oom
 *
 * doom -> Trie
 *
 * F:
 * .[b->F]:...........................[f->B]:
 * .......[a->B]:...........................doom,foo,quark,quux,rex
 * .............ar,ark,az,oom
 */

    using TForkValues = TVector<ui64>;

    struct TFork {
        TRefs ForksAndBuckets;
        ui64 Value;

        TFork()
            : Value(INVALID_VALUE)
        {
        }

        void Insert(TRef ref) {
            ForksAndBuckets.insert(LowerBound(ForksAndBuckets.begin(), ForksAndBuckets.end(), ref), ref);
        }

        TRef GetAddress(ui8 key) const {
            TRefs::const_iterator it = LowerBound(ForksAndBuckets.begin(), ForksAndBuckets.end(), TRef(key));

            if (it != ForksAndBuckets.end() && (ui8)it->Char == (ui8)key) {
                return *it;
            }

            if (it != ForksAndBuckets.begin() && (it - 1)->IsBucket()) {
                return *(it - 1);
            }

            if (it != ForksAndBuckets.end() && it->IsBucket()) {
                return *it;
            }

            return TRef();
        }

        void Remove(TRef ref) {
            TRefs::iterator it = FindIf(ForksAndBuckets.begin(), ForksAndBuckets.end(),
                                        std::bind(TRef::EqualTargets, ref, std::placeholders::_1));
            Y_VERIFY(it != ForksAndBuckets.end(), " ");
            ForksAndBuckets.erase(it);
        }
    };

#pragma pack(1)
    class Y_PACKED TBucketEntry {
        enum {
            LengthBits = 26,            //64Mb
            ValueBits = 64 - LengthBits //~256 * 10^9
        };

        const char* Suffix;
        ui64 Length : LengthBits;
        ui64 Value : ValueBits;

    public:
        TBucketEntry(TStringBuf suff = "", ui64 val = INVALID_VALUE)
            : Suffix()
            , Length()
            , Value()
        {
            SetSuffix(suff);
            SetValue(val);
        }

        TStringBuf GetSuffix() const {
            return TStringBuf(Suffix, Length);
        }

        char Front() const {
            return Suffix[0];
        }

        void PopFront() {
            Y_VERIFY(Length, " ");
            ++Suffix;
            --Length;
        }

        ui64 Size() const {
            return Length;
        }

        bool Empty() const {
            return !Length;
        }

        void SetSuffix(TStringBuf s) {
            Y_VERIFY(!s.empty(), "empty suffix");
            Y_VERIFY(s.size() <= Mask64(LengthBits), "too big key");
            Suffix = s.data();
            Length = s.size();
        }

        ui64 GetValue() const {
            return Value;
        }

        void SetValue(ui64 v) {
            Y_VERIFY(v == INVALID_VALUE || v < Mask64(ValueBits), "too many entries");
            Value = v;
        }

        friend bool operator<(const TBucketEntry& a, const TBucketEntry& b) {
            return a.GetSuffix() < b.GetSuffix();
        }
    };
#pragma pack()

    static_assert(sizeof(TBucketEntry) == 8 + sizeof(char*), "expect sizeof(TBucketEntry) == 8 + sizeof(char*)");

    using TBucketEntries = TVector<TBucketEntry>;
    using TCharVector = TVector<ui8>;

    struct TBucket {
        TBucketEntries Entries;

        void Reset() {
            TBucketEntries().swap(Entries);
        }

        const TBucketEntry* FindEntry(TStringBuf suffix) const {
            TBucketEntry entry(suffix);
            TBucketEntries::const_iterator iter = LowerBound(Entries.begin(), Entries.end(), entry);

            return iter == Entries.end() || iter->GetSuffix() != suffix ? nullptr : &*iter;
        }

        TBucketEntry* Insert(TStringBuf suffix, ui64 value) {
            Y_VERIFY(!suffix.empty(), " ");

            TBucketEntry entry(suffix, value);
            TBucketEntries::iterator iter = LowerBound(Entries.begin(), Entries.end(), entry);

            return iter != Entries.end() && iter->GetSuffix() == suffix ? nullptr : Entries.insert(iter, entry);
        }

        bool NeedsSplit(ui32 maxsize) {
            return Entries.size() > maxsize && Entries.front().Front() != Entries.back().Front();
        }

        bool NeedsPrefixExtraction(ui32 maxsize) {
            return Entries.size() > maxsize && Entries.front().Front() == Entries.back().Front();
        }

        void ExtractPrefix(TCharVector* poppedChars, TForkValues* poppedValues);

        void Split(TBucket* rightBucket);
    };

    using TBuckets = NPagedVector::TPagedVector<TBucket>;
    using TForks = NPagedVector::TPagedVector<TFork>;

    class TSolarTrieBuilder::TImpl: public TAtomicRefCount<TImpl> {
        TSolarTrieConf Conf;

        NIndexedRegion::TCompoundIndexedRegion RawKeys;
        NIndexedRegion::TCompoundIndexedRegion RawValues;

        NIndexedRegion::TCompoundIndexedRegion KeysReady;

        TBuffer KeyCompressBuffer;
        TBuckets Buckets;
        TForks Forks;

        ui64 DataSize = 0;
        bool Finalized = false;

    public:
        TImpl(const TSolarTrieConf& conf)
            : Conf(conf)
            , RawKeys(true, conf.RawValuesBlockSize, conf.RawValuesLengthCodec, conf.RawValuesBlockCodec)
            , RawValues(!!conf.KeyCodec || conf.UnsortedKeys,
                        conf.RawValuesBlockSize, conf.RawValuesLengthCodec, conf.RawValuesBlockCodec)
            , KeysReady(true,
                        conf.RawValuesBlockSize, conf.RawValuesLengthCodec, conf.RawValuesBlockCodec) {
            Forks.emplace_back();
            Y_VERIFY(conf.BucketMaxPrefixSize > 1 && conf.BucketMaxPrefixSize < conf.BucketMaxSize && (!conf.RawValuesBlockCodec || !conf.RawValuesBlockCodec->Traits().NeedsTraining), " ");
        }

        void AddKey(TStringBuf key) {
            if (!Conf.KeyCodec || Conf.KeyCodec->Traits().NeedsTraining) {
                RawKeys.PushBack(key);
                DataSize += key.size();
            } else {
                KeyCompressBuffer.Clear();
                Conf.KeyCodec->Encode(key, KeyCompressBuffer);
                RawKeys.PushBack(TStringBuf{KeyCompressBuffer.data(), KeyCompressBuffer.size()});
                DataSize += KeyCompressBuffer.Size();
            }
        }

        char* Add(TStringBuf key, TStringBuf val) {
            AddKey(key);
            DataSize += val.size();
            return const_cast<char*>(RawValues.PushBack(val).begin());
        }
        // return value Buffer to be filled
        char* Add(TStringBuf key, ui32 vsz) {
            AddKey(key);
            DataSize += vsz;
            return const_cast<char*>(RawValues.PushBackReserve(vsz).begin());
        }

        void Finalize();

        TBlob Compact();

        const TBuckets& GetBuckets() const {
            return Buckets;
        }

        const TForks& GetForks() const {
            return Forks;
        }

        const TSolarTrieConf& GetConf() const {
            return Conf;
        }

        const TFork& GetRoot() const {
            return Forks[0];
        }

        void Dump(IOutputStream& out) const;

    private:
        void AddOffset(TStringBuf key, ui64 val);

        TRef NewBucket(ui8 ch, TBucket** bucket) {
            TRef ref = TRef::NewBucket(ch, Buckets.size());

            Buckets.emplace_back();
            *bucket = &Buckets.back();

            return ref;
        }

        TRef NewFork(TFork** frk, ui8 ch, ui64 val) {
            TRef ref = TRef::NewFork(ch, Forks.size());

            Forks.emplace_back();
            *frk = &Forks.back();
            (*frk)->Value = val;

            return ref;
        }

        void Split(TFork* parent, TBucket* leaf, TRef current);

        void ExtractPrefix(TFork* parent, TBucket* leaf, TRef current);

        TRef LocateBucket(TStringBuf& key, const TFork*& parent) const;

        void Finalize(TFork& f, TForks& forks, TBuckets& b) const;

        void CompactFork(NIndexedRegion::TBigIndexedRegion& forks, NIndexedRegion::TBigIndexedRegion& values,
                         NIndexedRegion::TAdaptiveSmallRegion& valbuff,
                         TBuffer& sbuff, TBuffer& fbuff, TBuffer& bbuff) const;

        std::pair<size_t, size_t> CompactBucket(NIndexedRegion::TBigIndexedRegion& buckets,
                                                NIndexedRegion::TAdaptiveSmallRegion& suffixbuff,
                                                NIndexedRegion::TAdaptiveSmallRegion& valuebuff,
                                                TBuffer& sbuff, TBuffer& dbuff, TBucket& bbuff,
                                                TVector<TBuffer>& d);

        void PrintWarnMessage(const char* mess) const {
            if (Conf.Verbose) {
                Clog << mess << Endl;
            }
        }

        void PrintStatusMessage(const char* mess) const {
            if (Conf.Verbose) {
                Clog << Sprintf("%-110s RSS=%uM", mess, (ui32)(TRusage::Get().MaxRss >> 20)) << Endl;
            }
        }
    };

}
