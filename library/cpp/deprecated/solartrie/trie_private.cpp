#include "trie_private.h"

#include <util/generic/bitops.h>
#include <util/stream/format.h>
#include <util/string/hex.h>

namespace NSolarTrie {
    TSolarTrie::TSolarTrie() {
    }
    TSolarTrie::TSolarTrie(TBlob b)
        : Impl(new TImpl)
    {
        Impl->Wrap(b);
    }
    TSolarTrie::TSolarTrie(const TSolarTrie& t)
        : Impl(t.Impl)
    {
    }
    TSolarTrie& TSolarTrie::operator=(const TSolarTrie& t) {
        Impl = t.Impl;
        return *this;
    }
    TSolarTrie::~TSolarTrie() {
    }

    TSolarTrie::TConstIterator::TConstIterator()
        : Impl(new TImpl)
    {
    }
    TSolarTrie::TConstIterator::TConstIterator(const TConstIterator& it)
        : Impl(it.Impl)
    {
    }

    TSolarTrie::TConstIterator& TSolarTrie::TConstIterator::operator=(const TConstIterator& it) {
        Impl = it.Impl;
        return *this;
    }

    TSolarTrie::TConstIterator::~TConstIterator() {
    }

    void TSolarTrie::Init(TBlob b) {
        Impl = new TImpl;
        Impl->Wrap(b);
    }

    TBlob TSolarTrie::Data() const {
        return !Impl ? TBlob() : Impl->Memory;
    }

    bool TSolarTrie::Get(TStringBuf key, TBuffer& val) const {
        return Impl->Get(key, val);
    }

    bool TSolarTrie::Get(TStringBuf key, TBuffer& val, TBuffer& auxbuf) const {
        return Impl->Get(key, val, auxbuf);
    }

    size_t TSolarTrie::Size() const {
        return Impl->Size();
    }

    bool TSolarTrie::Empty() const {
        return Impl->Empty();
    }

    const TSolarTrieConf& TSolarTrie::GetConf() const {
        return Impl->GetConf();
    }

    TSolarTrie::TConstIterator TSolarTrie::Iterator() const {
        return Impl->Iterator();
    }

    void TSolarTrie::TConstIterator::TImpl::Init(const TSolarTrie::TImpl* t) {
        Exausted = false;
        Trie = t;
        CurrentBucket.Reset();
        CurrentBucket.Init(t->GetConf());
        CurrentValues.Init(t->GetConf().ValueLengthCodec, t->GetConf().ValueBlockCodec);
        CurrentValues.Clear();
        TrieCursor.clear();
        TrieCursor.reserve(16);
        BucketCursor = 0;
    }

    bool TSolarTrie::TConstIterator::TImpl::Next() {
        if (Exausted)
            return false;

        KeyBufferValid = ValBufferValid = false;

        if (TrieCursor.empty()) {
            if (Trie->GetForks().Empty()) {
                Exausted = true;
                return false;
            }

            Trie->PushFork(TrieCursor, 0);
            return Next();
        }

        const TCompactFork& f = TrieCursor.back().first;
        TRef r0 = TrieCursor.back().second;

        if (r0.IsBucket() && BucketCursor + 1 < CurrentBucket.Size()) {
            ++BucketCursor;
            return true;
        }

        TRef r1 = f.Next(r0);

        if (!r1) {
            TrieCursor.pop_back();

            if (TrieCursor.empty())
                Exausted = true;

            return Next();
        }

        TrieCursor.back().second = r1;

        if (r1.IsValue()) {
            return true;
        }

        if (r1.IsBucket()) {
            PrepareBucket(r1.Offset);
            return true;
        }

        if (r1.IsFork()) {
            Trie->PushFork(TrieCursor, r1.Offset);
            return Next();
        }

        Y_FAIL(" ");
        return false;
    }

    void TSolarTrie::TConstIterator::TImpl::PrepareBucket(ui32 offset) {
        BucketCursor = 0;
        CurrentBucket.Decode(Trie->GetBuckets()[offset]);
    }

    bool TSolarTrie::TConstIterator::TImpl::CurrentKey(TBuffer& key) const {
        if (!HasCurrent())
            return false;

        const TSolarTrieConf& c = Trie->GetConf();

        TRef r = TrieCursor.back().second;
        TmpBuffer.Clear();

        const size_t sz = TrieCursor.size();
        for (size_t i = 0; i < sz && TrieCursor[i].second.IsFork(); ++i) {
            TmpBuffer.Append(TrieCursor[i].second.Char);
        }

        if (r.IsBucket()) {
            if (!c.SuffixCodec) {
                TStringBuf m = CurrentBucket.GetSuffix(BucketCursor);
                TmpBuffer.Append(m.data(), m.size());
            } else {
                key.Clear();
                c.SuffixCodec->Decode(CurrentBucket.GetSuffix(BucketCursor), key);
                TmpBuffer.Append(key.Data(), key.Size());
            }

            if (!c.KeyCodec) {
                key.Assign(TmpBuffer.Data(), TmpBuffer.Size());
            } else {
                key.Clear();
                c.KeyCodec->Decode(TStringBuf{TmpBuffer.data(), TmpBuffer.size()}, key);
            }

            return true;
        }

        if (r.IsValue()) {
            if (!c.KeyCodec) {
                key.Assign(TmpBuffer.Data(), TmpBuffer.Size());
            } else {
                key.Clear();
                c.KeyCodec->Decode(TStringBuf{TmpBuffer.data(), TmpBuffer.size()}, key);
            }

            return true;
        }

        Y_FAIL(" ");
        return false;
    }

    bool TSolarTrie::TConstIterator::TImpl::CurrentVal(TBuffer& val) const {
        if (!HasCurrent())
            return false;

        const TSolarTrieConf& c = Trie->GetConf();

        TRef r = TrieCursor.back().second;

        if (r.IsBucket()) {
            if (!CurrentBucket.FilledValues)
                CurrentBucket.InitValues();

            if (!c.ValueCodec) {
                TStringBuf m = CurrentBucket.GetValue(BucketCursor);
                val.Assign(m.data(), m.size());
            } else {
                val.Clear();
                c.ValueCodec->Decode(CurrentBucket.GetValue(BucketCursor), val);
            }

            return true;
        }

        if (r.IsValue()) {
            CurrentValues.Decode(Trie->GetForkValues()[r.Offset / c.ForkValuesBlockSize]);
            TStringBuf mem = CurrentValues.Get(r.Offset % c.ForkValuesBlockSize);
            if (!c.ValueCodec) {
                val.Assign(mem.data(), mem.size());
            } else {
                val.Clear();
                c.ValueCodec->Decode(mem, val);
            }

            return true;
        }

        Y_FAIL(" ");
        return false;
    }

    void TSolarTrie::TImpl::Dump(IOutputStream& out) const {
        TBuffer vb;
        for (ui32 i = 0; i < CompactForks.Size(); ++i) {
            TCompactFork f;
            f.Decode(CompactForks[i]);

            out << "Fork #" << i << Endl;

            if (f.HasValue()) {
                NIndexedRegion::TAdaptiveSmallRegion v(Conf.ValueLengthCodec, Conf.ValueBlockCodec);
                v.Decode(ForkValues[f.ValueStart / Conf.ForkValuesBlockSize]);
                TStringBuf vr = v.Get(f.ValueStart % Conf.ForkValuesBlockSize);
                if (!!Conf.ValueCodec) {
                    vb.Clear();
                    Conf.ValueCodec->Decode(vr, vb);
                    vr = TStringBuf{vb.data(), vb.size()};
                }
                out << "  -> " << vr << Endl;
            } else {
                out << "  -x" << Endl;
            }

            for (ui32 j = 0; j < f.ForksIndex.size(); ++j) {
                if (!Conf.KeyCodec) {
                    out << "  " << (char)f.ForksIndex[j] << " -> " << f.ForksStart + j << Endl;
                } else {
                    out << "  " << Bin(ReverseBits((ui8)f.ForksIndex[j]), HF_FULL) << " -> " << f.ForksStart + j << Endl;
                }
            }

            for (ui32 j = 0; j < f.BucketsIndex.size(); ++j) {
                if (!Conf.KeyCodec) {
                    out << "  " << (char)(f.BucketsIndex[j].Buf & 0x00FF)
                        << ":" << (char)((f.BucketsIndex[j].Buf & 0xFF00) >> 8)
                        << " -> " << f.BucketsStart + j << Endl;
                } else {
                    out
                        << "  " << Bin(ReverseBits(ui8((f.BucketsIndex[j].Buf & 0x00FF))), HF_FULL)
                        << ":" << Bin(ReverseBits(ui8((f.BucketsIndex[j].Buf & 0xFF00) >> 8)), HF_FULL)
                        << " -> " << f.BucketsStart + j << Endl;
                }
            }
        }

        TCompactBucket b;
        b.Init(Conf);
        for (ui32 i = 0; i < Buckets.Size(); ++i) {
            b.Decode(Buckets[i]);
            b.InitValues();

            out << "Bucket #" << i << Endl;

            TBuffer d;
            for (ui32 j = 0; j < b.Size(); ++j) {
                TStringBuf r = b.GetSuffix(j);

                if (Conf.SuffixCodec.Get()) {
                    d.Clear();
                    Conf.SuffixCodec->Decode(r, d);
                    r = TStringBuf{d.data(), d.size()};
                }

                if (!Conf.KeyCodec) {
                    out << "  " << r;
                } else {
                    out << "  " << HexEncode(r.data(), r.size());
                }

                r = b.GetValue(j);

                if (Conf.ValueCodec.Get()) {
                    d.Clear();
                    Conf.ValueCodec->Decode(r, d);
                    r = TStringBuf{d.data(), d.size()};
                }

                out << " -> " << r << Endl;
            }
        }
    }

    bool TSolarTrie::TConstIterator::Next() {
        return Impl->Next();
    }

    bool TSolarTrie::TConstIterator::Current(TBuffer& key, TBuffer& val) const {
        return Impl->Current(key, val);
    }

    bool TSolarTrie::TConstIterator::HasCurrent() const {
        return Impl->HasCurrent();
    }

    bool TSolarTrie::TConstIterator::CurrentKey(TBuffer& key) const {
        return Impl->CurrentKey(key);
    }

    bool TSolarTrie::TConstIterator::CurrentVal(TBuffer& val) const {
        return Impl->CurrentVal(val);
    }

    TStringBuf TSolarTrie::TConstIterator::CurrentKey() const {
        return Impl->CurrentKey();
    }

    TStringBuf TSolarTrie::TConstIterator::CurrentVal() const {
        return Impl->CurrentVal();
    }

    void TSolarTrie::TConstIterator::Restart() {
        Impl->Restart();
    }

    TSolarTrie::TConstIterator TSolarTrie::TConstIterator::Clone() const {
        TConstIterator it;
        Impl->CopyTo(*it.Impl);
        return it;
    }

}
