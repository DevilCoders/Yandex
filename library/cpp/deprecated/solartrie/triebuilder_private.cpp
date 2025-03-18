#include "triebuilder_private.h"

namespace NSolarTrie {
    using namespace NCodecs;
    using namespace NIndexedRegion;

    static const float MB = 1024 * 1024;

    void TBucket::ExtractPrefix(TCharVector* poppedChars, TForkValues* poppedValues) {
        if (Entries.empty() || Entries.front().Front() != Entries.back().Front())
            return;

        TBucketEntries::iterator iter = Entries.begin();

        poppedChars->push_back(iter->Front());

        while (iter != Entries.end()) {
            iter->PopFront();

            if (iter->Empty()) {
                poppedValues->push_back(iter->GetValue());
                iter = Entries.erase(iter);
            } else
                iter++;
        }

        if (poppedChars->size() != poppedValues->size())
            poppedValues->push_back(INVALID_VALUE);

        Y_VERIFY(poppedValues->size() == poppedChars->size(), " ");

        ExtractPrefix(poppedChars, poppedValues);
    }

    void TBucket::Split(TBucket* rightBucket) {
        Y_VERIFY(!Entries.empty(), " ");

        size_t mid = Entries.size() / 2;
        char midc = Entries[mid].Front();

        size_t left = mid;
        size_t right = mid;

        while (left && Entries[left - 1].Front() == midc)
            --left;

        while (right < Entries.size() && Entries[right].Front() == midc)
            ++right;

        // .....aaaaaaaaaa....
        // .....|...|.....|...
        // .....L...M.....R...

        Y_VERIFY(left || right < Entries.size(), " ");

        if (!left)
            mid = right;
        else if (right == Entries.size())
            mid = left;
        else
            mid = right - mid > mid - left ? left : right;

        rightBucket->Entries.insert(rightBucket->Entries.end(), Entries.begin() + mid, Entries.end());
        Entries.erase(Entries.begin() + mid, Entries.end());

        Y_VERIFY(!Entries.empty() && !rightBucket->Entries.empty(), " ");
        Y_VERIFY(!Entries[0].Empty() && !rightBucket->Entries[0].Empty(), " ");
    }

    TRef TSolarTrieBuilder::TImpl::LocateBucket(TStringBuf& key, const TFork*& parent) const {
        parent = &Forks[0];
        TRef current = TRef::NewFork(0, 0);

        while (!key.empty() && current.IsFork()) {
            parent = &Forks[current.Offset];
            current = parent->GetAddress(key[0]);

            if (current.IsFork())
                key.Skip(1);
        }

        return current;
    }

    void TSolarTrieBuilder::TImpl::AddOffset(TStringBuf kkey, ui64 v) {
        Y_VERIFY(!Finalized, " ");

        TFork* parent = nullptr;
        TStringBuf key = kkey;
        TRef current = LocateBucket(key, (const TFork*&)parent);

        TBucket* leaf = nullptr;

        if (!current) {
            Y_VERIFY(!key.empty(), " ");
            parent->Insert(current = NewBucket(key[0], &leaf));
        } else if (current.IsBucket()) {
            Y_VERIFY(!key.empty(), " ");
            leaf = &Buckets[current.Offset];
        } else {
            Y_VERIFY(key.empty() && current.IsFork(), " ");

            parent = &Forks[current.Offset];

            if (parent->Value != INVALID_VALUE)
                return;

            parent->Value = v;
            return;
        }

        if (TBucketEntry* e = leaf->Insert(key, 0)) {
            e->SetSuffix(kkey.Skip(kkey.size() - key.size()));
            e->SetValue(v);
        } else
            return;

        if (leaf->NeedsPrefixExtraction(Conf.BucketMaxPrefixSize))
            ExtractPrefix(parent, leaf, current);

        if (leaf->NeedsSplit(Conf.BucketMaxSize))
            Split(parent, leaf, current);

        return;
    }

    void TSolarTrieBuilder::TImpl::Split(TFork* parent, TBucket* leaf, TRef current) {
        TBucket* right = nullptr;
        TRef rightAddress = NewBucket(0, &right);

        parent->Remove(current);
        leaf->Split(right);

        current.Char = (ui8)leaf->Entries[0].Front();
        parent->Insert(current);

        rightAddress.Char = (ui8)right->Entries[0].Front();
        parent->Insert(rightAddress);

        Y_VERIFY(current.Char <= rightAddress.Char, " ");

        if (leaf->NeedsPrefixExtraction(Conf.BucketMaxPrefixSize))
            ExtractPrefix(parent, leaf, current);

        if (right->NeedsPrefixExtraction(Conf.BucketMaxPrefixSize))
            ExtractPrefix(parent, right, rightAddress);
    }

    void TSolarTrieBuilder::TImpl::ExtractPrefix(TFork* parent, TBucket* leaf, TRef current) {
        TCharVector poppedChars;
        TForkValues poppedValues;

        parent->Remove(current);
        leaf->ExtractPrefix(&poppedChars, &poppedValues);

        TFork* entry = nullptr;
        for (ui32 i = 0; i < poppedValues.size(); i++) {
            parent->Insert(NewFork(&entry, poppedChars[i], poppedValues[i]));
            parent = entry;
        }

        if (!leaf->Entries.empty()) {
            current.Char = (ui8)leaf->Entries[0].Front();
            parent->Insert(current);
        }
    }

    template <bool GetValue, bool GetLength, typename TLen>
    struct TGetBucket {
        TCodecPtr Codec;
        TBuffer Buffer;
        TBuffer CodecBuffer;
        const TCompoundIndexedRegion& Keys;
        const TCompoundIndexedRegion& Values;

        TGetBucket(const TCompoundIndexedRegion& keys,
                   const TCompoundIndexedRegion& vals, const TSolarTrieConf& c)
            : Codec(GetValue ? c.ValueCodec : c.SuffixCodec)
            , Keys(keys)
            , Values(vals)
        {
        }

        TStringBuf operator()(TBuckets::const_iterator iter) {
            Buffer.Clear();

            for (auto entrie : iter->Entries) {
                TStringBuf r;

                if (GetValue) {
                    r = Values[entrie.GetValue()];
                } else {
                    r = entrie.GetSuffix();
                }

                if (Codec.Get()) {
                    CodecBuffer.Clear();
                    Codec->Encode(r, CodecBuffer);
                    r = TStringBuf{CodecBuffer.data(), CodecBuffer.size()};
                }

                if (GetLength) {
                    TLen size = r.size();
                    Buffer.Append((const char*)&size, sizeof(size));
                } else {
                    Buffer.Append(r.data(), r.size());
                }
            }

            return TStringBuf{Buffer.data(), Buffer.size()};
        }
    };

    using TGetSufBlocks = TGetBucket<false, false, ui64>;
    using TGetSufLens32 = TGetBucket<false, true, ui32>;
    using TGetSufLens64 = TGetBucket<false, true, ui64>;

    using TGetValBlocks = TGetBucket<true, false, ui64>;
    using TGetValLens32 = TGetBucket<true, true, ui32>;
    using TGetValLens64 = TGetBucket<true, true, ui64>;

    template <class TChild>
    class TBucketIteratorBase {
        const TBuckets& Buckets;
        size_t Bucket = 0;
        size_t Entry = 0;

    protected:
        TBucketIteratorBase(const TBuckets& bucks, size_t buckNum)
            : Buckets(bucks)
            , Bucket(buckNum)
        {
        }

        const TBucket& GetBucket() const {
            Y_ENSURE_EX(Bucket < Buckets.size(), TTrieException());
            return Buckets[Bucket];
        }

        const TBucketEntry& GetEntry() const {
            const auto& buck = GetBucket();
            Y_ENSURE_EX(Entry < buck.Entries.size(), TTrieException());
            return buck.Entries[Entry];
        }

    public:
        bool operator==(const TChild& other) const {
            return Bucket == other.Bucket && Entry == other.Entry;
        }

        bool operator!=(const TChild& other) const {
            return !(*this == other);
        }

        TChild& operator++() {
            Y_ENSURE_EX(Bucket < Buckets.size(), TTrieException());

            Entry += 1;

            if (Entry >= GetBucket().Entries.size()) {
                Bucket += 1;
                Entry = 0;
            }

            return (TChild&)*this;
        }
    };

    class TSuffixIterator: public TBucketIteratorBase<TSuffixIterator> {
    public:
        TSuffixIterator(const TBuckets& bucks, size_t buckNum)
            : TBucketIteratorBase(bucks, buckNum)
        {
        }

        TStringBuf operator*() const {
            return GetEntry().GetSuffix();
        }
    };

    void TSolarTrieBuilder::TImpl::Finalize() {
        RawKeys.Commit();
        RawValues.Commit();
        Conf.ResetCodecs();

        PrintStatusMessage("Finalizing trie...");

        if (!!Conf.KeyCodec && Conf.KeyCodec->Traits().NeedsTraining) {
            PrintStatusMessage("  Training keys...");
            Conf.KeyCodec->Learn(RawKeys.Begin(), RawKeys.End());
            PrintStatusMessage("  Done");
        }

        if (!!Conf.ValueCodec && Conf.ValueCodec->Traits().NeedsTraining) {
            PrintStatusMessage("  Training values...");
            Conf.ValueCodec->Learn(RawValues.Begin(), RawValues.End());
            PrintStatusMessage("  Done");
        }

        {
            PrintStatusMessage("  Compressing keys...");

            if (!!Conf.KeyCodec && Conf.KeyCodec->Traits().NeedsTraining) {
                TBuffer k;
                const size_t sz = RawKeys.Size();
                for (size_t i = 0; i < sz; ++i) {
                    k.Clear();
                    Conf.KeyCodec->Encode(RawKeys[i], k);
                    RawKeys.DisposePreviousBlocks(i);
                    KeysReady.PushBack(TStringBuf{k.data(), k.size()});
                }
                RawKeys.Clear();
                KeysReady.Commit();
            } else {
                KeysReady.Swap(RawKeys);
            }

            PrintStatusMessage("  Done");
        }

        {
            PrintStatusMessage("  Assembling trie...");

            const size_t sz = KeysReady.Size();
            for (size_t i = 0; i < sz; ++i) {
                AddOffset(KeysReady[i], i);
            }

            PrintStatusMessage("  Done");
        }

        {
            PrintStatusMessage("  Topologically sorting forks and buckets...");

            TForks forks;
            TBuckets buckets;

            forks.push_back(Forks[0]);
            Finalize(forks.front(), forks, buckets);

            Forks.swap(forks);
            Buckets.swap(buckets);

            PrintStatusMessage("  Done");
        }

        if (!!Conf.SuffixCodec && Conf.SuffixCodec->Traits().NeedsTraining) {
            PrintStatusMessage("  Training suffix codec...");
            Conf.SuffixCodec->Learn(TSuffixIterator{Buckets, 0}, TSuffixIterator{Buckets, Buckets.size()});
            PrintStatusMessage("  Done");
        }

        if (!!Conf.SuffixBlockCodec && Conf.SuffixBlockCodec->Traits().NeedsTraining) {
            PrintStatusMessage("  Training suffix block codec...");

            TGetSufBlocks gb(RawKeys, RawValues, Conf);
            Conf.SuffixBlockCodec->Learn(Buckets.begin(), Buckets.end(), gb);

            PrintStatusMessage("  Done");
        }

        if (!!Conf.SuffixLengthCodec && Conf.SuffixLengthCodec->Traits().NeedsTraining) {
            PrintStatusMessage("  Training suffix length codec...");

            if (TAdaptiveSmallRegion::Use64BitLengths(Conf.SuffixLengthCodec.Get())) {
                TGetSufLens64 gb(RawKeys, RawValues, Conf);
                Conf.SuffixLengthCodec->Learn(Buckets.begin(), Buckets.end(), gb);
            } else {
                TGetSufLens32 gb(RawKeys, RawValues, Conf);
                Conf.SuffixLengthCodec->Learn(Buckets.begin(), Buckets.end(), gb);
            }

            PrintStatusMessage("  Done");
        }

        if (!!Conf.ValueBlockCodec && Conf.ValueBlockCodec->Traits().NeedsTraining) {
            PrintStatusMessage("  Training value block codec...");

            TGetValBlocks gb(RawKeys, RawValues, Conf);
            Conf.ValueBlockCodec->Learn(Buckets.begin(), Buckets.end(), gb);

            PrintStatusMessage("  Done");
        }

        if (!!Conf.ValueLengthCodec && Conf.ValueLengthCodec->Traits().NeedsTraining) {
            PrintStatusMessage("  Training value length codec...");

            if (TAdaptiveSmallRegion::Use64BitLengths(Conf.ValueLengthCodec.Get())) {
                TGetValLens64 gb(RawKeys, RawValues, Conf);
                Conf.ValueLengthCodec->Learn(Buckets.begin(), Buckets.end(), gb);
            } else {
                TGetValLens32 gb(RawKeys, RawValues, Conf);
                Conf.ValueLengthCodec->Learn(Buckets.begin(), Buckets.end(), gb);
            }

            PrintStatusMessage("  Done");
        }

        PrintStatusMessage("Done");
        Finalized = true;
    }

    void TSolarTrieBuilder::TImpl::Finalize(TFork& f, TForks& forks, TBuckets& buckets) const {
        for (auto& forksAndBucket : f.ForksAndBuckets) {
            size_t off = forksAndBucket.Offset;

            if (forksAndBucket.IsBucket()) {
                forksAndBucket.Offset = buckets.size();
                buckets.push_back(Buckets[off]);
            } else {
                forksAndBucket.Offset = forks.size();
                forks.push_back(Forks[off]);
            }
        }

        for (auto forksAndBucket : f.ForksAndBuckets) {
            if (forksAndBucket.IsFork())
                Finalize(forks[forksAndBucket.Offset], forks, buckets);
        }
    }

    static TStringBuf GetValue(size_t idx, const TCompoundIndexedRegion& vals, const TCodecPtr& codec, TBuffer& buff) {
        TStringBuf r = vals[idx];
        if (!!codec) {
            buff.Clear();
            codec->Encode(r, buff);
            r = TStringBuf{buff.data(), buff.size()};
        }
        return r;
    }

    void TSolarTrieBuilder::TImpl::CompactFork(
        TBigIndexedRegion& forks, TBigIndexedRegion& values,
        TAdaptiveSmallRegion& valblock,
        TBuffer& sbuff, TBuffer& fbuff, TBuffer& bbuff) const {
        const TFork& f = Forks[forks.Size()];

        TRef firstfork;
        TRef firstbuck;

        sbuff.Clear();
        fbuff.Clear();
        bbuff.Clear();

        for (auto forksAndBucket : f.ForksAndBuckets) {
            if (forksAndBucket.IsFork()) {
                if (!firstfork)
                    firstfork = forksAndBucket;

                fbuff.Append((ui8)forksAndBucket.Char);
            } else {
                if (!firstbuck)
                    firstbuck = forksAndBucket;

                const TBucket& b = Buckets[forksAndBucket.Offset];

                if (b.Entries.empty()) {
                    bbuff.Append((ui8)'\xFF');
                    bbuff.Append((ui8)'\0');
                } else {
                    bbuff.Append((ui8)b.Entries.front().Front());
                    bbuff.Append((ui8)b.Entries.back().Front());
                }
            }
        }

        Y_VERIFY(valblock.Size() < Conf.ForkValuesBlockSize, " ");

        {
            NBitIO::TBitOutputVector<TBuffer> v(&sbuff);
            v.WriteWords<7>(firstfork ? firstfork.Offset : 0);
            v.WriteWords<7>(firstbuck ? firstbuck.Offset : 0);
            v.WriteWords<7>(f.Value != INVALID_VALUE ? values.Size() * Conf.ForkValuesBlockSize + valblock.Size() + 1 : 0);
            v.WriteWords<7>(fbuff.Size());
        }

        sbuff.Append(fbuff.Data(), fbuff.Size());
        sbuff.Append(bbuff.Data(), bbuff.Size());
        forks.PushBack(TStringBuf{sbuff.data(), sbuff.size()});

        if (f.Value != INVALID_VALUE) {
            valblock.PushBack(GetValue(f.Value, RawValues, Conf.ValueCodec, fbuff));
        }
    }

    std::pair<size_t, size_t> TSolarTrieBuilder::TImpl::CompactBucket(
        TBigIndexedRegion& buckets,
        TAdaptiveSmallRegion& suffixbuff, TAdaptiveSmallRegion& valuebuff,
        TBuffer& sbuff, TBuffer& dbuff, TBucket& bbuff, TVector<TBuffer>& d) {
        const TBucket* bucket = &Buckets[buckets.Size()];

        std::pair<size_t, size_t> sizes;
        suffixbuff.Clear();
        valuebuff.Clear();
        sbuff.Clear();
        dbuff.Clear();
        bbuff.Entries.clear();

        if (Conf.SuffixCodec.Get()) {
            d.clear();

            for (auto entrie : bucket->Entries) {
                d.emplace_back();
                Conf.SuffixCodec->Encode(entrie.GetSuffix(), d.back());

                bbuff.Entries.push_back(TBucketEntry(
                    TStringBuf(d.back().Begin(), d.back().End()),
                    entrie.GetValue()));
            }

            Sort(bbuff.Entries.begin(), bbuff.Entries.end());
            bucket = &bbuff;
        }

        for (auto entrie : bucket->Entries) {
            suffixbuff.PushBack(entrie.GetSuffix());
            valuebuff.PushBack(GetValue(entrie.GetValue(), RawValues, Conf.ValueCodec, sbuff));
        }

        sbuff.Clear();
        suffixbuff.Commit();
        suffixbuff.Encode(sbuff);
        sizes.first = sbuff.Size();

        {
            NBitIO::TBitOutputVector<TBuffer> bout(&dbuff);
            bout.WriteWords<7>(sbuff.Size());
        }

        dbuff.Append(sbuff.Data(), sbuff.Size());

        valuebuff.Commit();

        sbuff.Clear();
        valuebuff.Encode(sbuff);
        sizes.second = sbuff.Size();
        dbuff.Append(sbuff.Data(), sbuff.Size());

        buckets.PushBack(TStringBuf{dbuff.data(), dbuff.size()});
        return sizes;
    }

    static ui64 WriteBigIndexedRegionFast(TBufferOutput& bout, TBigIndexedRegion& region) {
        ui64 noff = bout.Buffer().Size();
        ::Save(&bout, (ui64)0);
        ui64 nlen = bout.Buffer().Size() - noff;
        region.Save(&bout);
        ui64 rsz = bout.Buffer().Size() - noff - nlen;
        {
            TMemoryOutput mout(bout.Buffer().Begin() + noff, nlen);
            ::Save(&mout, (ui64)rsz);
        }
        region.Clear();
        return rsz;
    }

    TBlob TSolarTrieBuilder::TImpl::Compact() {
        using namespace NCodecs;
        Y_VERIFY(Finalized, " ");

        PrintStatusMessage("Compacting...");

        TBufferOutput bout;
        bout.Buffer().Reserve(DataSize);
        Conf.Save(&bout);

        {
            PrintStatusMessage("  Compacting forks...");

            TBigIndexedRegion forks;
            TBigIndexedRegion forkvalues;

            {
                TBuffer fbuff;
                TBuffer bbuff;
                TBuffer dbuff;
                TAdaptiveSmallRegion valblock(Conf.ValueLengthCodec, Conf.ValueBlockCodec);

                for (TForks::const_iterator it = Forks.begin(); it != Forks.end(); ++it) {
                    CompactFork(forks, forkvalues, valblock, fbuff, bbuff, dbuff);

                    if (valblock.Size() == Conf.ForkValuesBlockSize) {
                        valblock.Commit();
                        dbuff.Clear();
                        valblock.Encode(dbuff);
                        forkvalues.PushBack(TStringBuf{dbuff.data(), dbuff.size()});
                        valblock.Clear();
                    }
                }

                if (valblock.Size()) {
                    valblock.Commit();
                    dbuff.Clear();
                    valblock.Encode(dbuff);
                    forkvalues.PushBack(TStringBuf{dbuff.data(), dbuff.size()});
                    valblock.Clear();
                }
            }

            Forks.clear();
            size_t fcnt = forks.Size();
            forks.Commit();

            size_t hsz = bout.Buffer().Size();
            size_t fsz = WriteBigIndexedRegionFast(bout, forks);

            forkvalues.Commit();

            size_t fvl = WriteBigIndexedRegionFast(bout, forkvalues);

            TString s = Sprintf("  Done. Header takes %0.2fM. %lu forks take %0.2fM. Values take %0.2fM",
                                hsz / MB, fcnt, fsz / MB, fvl / MB);
            PrintStatusMessage(s.data());
        }

        {
            PrintStatusMessage("  Compacting buckets...");

            TBigIndexedRegion buckets;
            TBuffer sbuff;
            TBuffer dbuff;
            TBucket bbuff;
            TVector<TBuffer> d;
            size_t suffixes = 0;
            size_t values = 0;
            ui64 count = Buckets.size();
            ui64 entr = 0;
            {
                TAdaptiveSmallRegion suffixbuff(Conf.SuffixLengthCodec, Conf.SuffixBlockCodec);
                TAdaptiveSmallRegion valuebuff(Conf.ValueLengthCodec, Conf.ValueBlockCodec);

                for (auto& bucket : Buckets) {
                    entr += bucket.Entries.size();
                    std::pair<size_t, size_t> sz = CompactBucket(buckets, suffixbuff, valuebuff, sbuff, dbuff, bbuff, d);
                    suffixes += sz.first;
                    values += sz.second;
                    bucket.Reset();
                }
            }

            Buckets.clear();
            RawKeys.Clear();
            RawValues.Clear();

            buckets.Commit();

            size_t bsz = WriteBigIndexedRegionFast(bout, buckets);

            TString s = Sprintf("  Done. %lu buckets (av pop %0.2f) take %0.2fM (suffixes: %0.2fM, values: %0.2fM).",
                                count, float(entr) / Max<ui32>(1, count), bsz / MB, suffixes / MB, values / MB);
            PrintStatusMessage(s.data());
        }

        PrintStatusMessage("Done");

        return TBlob::FromBuffer(bout.Buffer());
    }

    void TSolarTrieBuilder::TImpl::Dump(IOutputStream& out) const {
        TBuffer v;

        for (TForks::const_iterator it = Forks.begin(); it != Forks.end(); ++it) {
            out << "Fork #" << it - Forks.begin() << ": " << Endl;

            TStringBuf r;

            if (it->Value != INVALID_VALUE) {
                r = RawValues[it->Value];

                if (Conf.ValueCodec.Get()) {
                    v.Clear();
                    Conf.ValueCodec->Decode(r, v);
                    r = TStringBuf{v.data(), v.size()};
                }

                out << "  -> " << r << Endl;
            } else {
                out << "  -x" << Endl;
            }

            for (TRefs::const_iterator rit = it->ForksAndBuckets.begin(); rit != it->ForksAndBuckets.end(); ++rit) {
                Y_ENSURE_EX(*rit, TTrieException());
                out << "  " << (char)rit->Char << " -> " << rit->Offset << (rit->IsBucket() ? "(B)" : "(F)") << Endl;
            }
        }

        for (TBuckets::const_iterator it = Buckets.begin(); it != Buckets.end(); ++it) {
            out << "Bucket #" << it - Buckets.begin() << ": " << Endl;

            for (auto entrie : it->Entries) {
                TStringBuf r = RawValues[entrie.GetValue()];

                if (Conf.ValueCodec.Get()) {
                    v.Clear();
                    Conf.ValueCodec->Decode(r, v);
                    r = TStringBuf{v.data(), v.size()};
                }

                out << "  " << entrie.GetSuffix() << " -> " << r << Endl;
            }
        }
    }

    TSolarTrieBuilder::TSolarTrieBuilder(TSolarTrieConf conf)
        : Impl(new TImpl(conf))
    {
    }

    TSolarTrieBuilder::~TSolarTrieBuilder() {
    }

    char* TSolarTrieBuilder::Add(TStringBuf key, TStringBuf value) {
        return Impl->Add(key, value);
    }

    char* TSolarTrieBuilder::Add(TStringBuf key, ui32 vSize) {
        return Impl->Add(key, vSize);
    }

    TBlob TSolarTrieBuilder::Compact() {
        Impl->Finalize();
        return Impl->Compact();
    }

}
