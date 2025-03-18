#pragma once

#include "trie_conf.h"
#include "memory_representation.h"

#include <library/cpp/codecs/tls_cache.h>
#include <library/cpp/containers/comptrie/comptrie_trie.h>

#include <util/generic/strbuf.h>
#include <util/stream/mem.h>

namespace NCodecTrie {
    template <typename TVal, typename TPacker>
    class TCodecTrieBuilder;

    template <typename TVal = TStringBuf, typename TPacker = TCompactTriePacker<TVal>>
    class TCodecTrie {
        typedef TCodecTrie<TVal, TPacker> TTrie;
        typedef TMemoryRepresentation<TVal, TPacker, true> TRepresentationSafe;
        typedef TMemoryRepresentation<TVal, TPacker, false> TRepresentationFast;

        typedef TCompactTrie<char, TVal, TPacker> TKeyVals;
        typedef TCompactTrie<char, TStringBuf> TKeyValsCoded;

        ui8 HasEmptyValue;

        TBlob EmptyValueBuffer;
        TVal EmptyValue;

        TBlob Memory;

        TKeyVals KeyVals;
        TKeyValsCoded KeyValsCoded;

        TCodecTrieConf Conf;

    public:
        typedef TCodecTrieBuilder<TVal, TPacker> TBuilder;

        class TConstIterator;

    public:
        TCodecTrie()
            : HasEmptyValue()
        {
        }
        TCodecTrie(TBlob b)
            : HasEmptyValue()
        {
            Init(b);
        }

        const TCodecTrieConf& GetConf() const {
            return Conf;
        }

        bool Get(TStringBuf key, TVal& value, TBuffer& decodebuff, TBuffer& valuebuff) const {
            return key.empty() ? GetEmpty(value) : FindValue(key, value, decodebuff, valuebuff);
        }

        bool GetValue(TStringBuf key, TVal& value, TBuffer& decodebuff, TBuffer& valuebuff) const {
            return DoGetValue(key, value, decodebuff, valuebuff);
        }

        bool Get(TStringBuf key, TVal& value, TBuffer& valuebuff) const {
            auto valBuffer = NCodecs::TBufferTlsCache::TlsInstance().Item();
            return key.empty() ? GetEmpty(value) : FindValue(key, value, valBuffer.Get(), valuebuff);
        }

        // use this method for compressed values owning their memory
        bool Get(TStringBuf key, TVal& value) const {
            auto tmpBuffer = NCodecs::TBufferTlsCache::TlsInstance().Item();
            auto valBuffer = NCodecs::TBufferTlsCache::TlsInstance().Item();
            return Get(key, value, tmpBuffer.Get(), valBuffer.Get());
        }

        bool Empty() const {
            return !HasEmptyValue && (!Conf.ValueCodec ? KeyVals.IsEmpty() : KeyValsCoded.IsEmpty());
        }

        size_t Size() const {
            return (!Conf.ValueCodec ? KeyVals.Size() : KeyValsCoded.Size());
        }

        const TBlob& Data() const {
            return Memory;
        }

        TConstIterator Iterator() const;

        bool SupportsPrefixLookups() const {
            return !Conf.KeyCodec;
        }

        bool HasPrefix(TStringBuf prefix) const {
            Y_ENSURE_EX(SupportsPrefixLookups(), TTrieException() << "prefix lookups are unsupported");
            return !(prefix && (Conf.ValueCodec ? KeyValsCoded.FindTails(prefix).IsEmpty() : KeyVals.FindTails(prefix).IsEmpty()));
        }

        // todo: pattern matching
        // todo: all the other functionality of the TCompactTrie

        void Init(TBlob b) {
            Memory = b;

            {
                TMemoryInput min(b.Data(), b.Size());
                Conf.Load(&min);

                ::Load(&min, HasEmptyValue);

                if (HasEmptyValue) {
                    ui64 evsz = 0;
                    ::Load(&min, evsz);
                    TBuffer tmp;
                    TBuffer carrier;

                    tmp.Resize(evsz);
                    min.LoadOrFail(tmp.Begin(), evsz);
                    TRepresentationSafe rep(&carrier);
                    rep.FromMemoryRegion(EmptyValue, TStringBuf{tmp.data(), tmp.size()});
                    EmptyValueBuffer = TBlob::FromBuffer(carrier);
                }

                b = b.SubBlob(b.Size() - min.Avail(), b.Size());
            }

            if (Conf.ValueCodec.Get()) {
                KeyValsCoded.Init(b);
            } else {
                KeyVals.Init(b);
            }
        }

    private:
        bool FindValue(TStringBuf key, TVal& value, TBuffer& buff, TBuffer& auxbuff) const {
            using namespace NCodecs;
            buff.Clear();

            if (Conf.KeyCodec.Get()) {
                Conf.KeyCodec->Encode(key, buff);
                key = TStringBuf(buff.Begin(), buff.End());
            }

            return DoGetValue(key, value, buff, auxbuff);
        }

        bool DoGetValue(TStringBuf key, TVal& value, TBuffer& buff, TBuffer& auxbuff) const {
            if (!Conf.ValueCodec) {
                return KeyVals.Find(key, &value);
            }

            TStringBuf sb;

            if (!KeyValsCoded.Find(key, &sb)) {
                return false;
            }

            buff.Clear();
            auxbuff.Clear();
            Conf.ValueCodec->Decode(sb, buff);
            TRepresentationFast rep(&auxbuff);
            rep.FromMemoryRegion(value, TStringBuf{buff.data(), buff.size()});
            return true;
        }

        bool GetEmpty(TVal& val) const {
            if (!HasEmptyValue) {
                return false;
            }

            val = EmptyValue;
            return true;
        }
    };

    template <typename TVal, typename TPacker>
    class TCodecTrie<TVal, TPacker>::TConstIterator {
        friend class TCodecTrie<TVal, TPacker>;

        const TTrie* Trie = nullptr;

        typename TKeyVals::TConstIterator Begin;
        typename TKeyVals::TConstIterator End;
        typename TKeyVals::TConstIterator It;

        TKeyValsCoded::TConstIterator BeginCoded;
        TKeyValsCoded::TConstIterator EndCoded;
        TKeyValsCoded::TConstIterator ItCoded;

        mutable TString CurrentKey_;
        mutable TVal CurrentVal_;
        mutable TBuffer Buffer;
        mutable TBuffer AuxBuffer;

        bool AtEmptyValue = false;
        mutable bool BufferedKey_ = false;
        mutable bool BufferedVal_ = false;

    public:
        TConstIterator() = default;

        bool Next() {
            if (!Trie)
                return false;

            BufferedKey_ = false;
            BufferedVal_ = false;

            return !Trie->Conf.ValueCodec ? DoNext(It, Begin, End) : DoNext(ItCoded, BeginCoded, EndCoded);
        }

        template <typename TTKey, typename TTVal>
        bool Current(TTKey& k, TTVal& v) const {
            TStringBuf kk;
            bool res = DoCurrent(kk, v, nullptr);
            k = kk;
            return res;
        }

        bool Current(TString& k, TVal& v, TBuffer& aux) const {
            TStringBuf kk;
            bool res = DoCurrent(kk, v, aux);
            k = kk;
            return res;
        }

        template <typename TTKey>
        bool CurrentKey(TTKey& k) const {
            bool res = DoCurrentKey();
            k = CurrentKey_;
            return res;
        }

        template <typename TTVal>
        bool CurrentVal(TTVal& v) const {
            bool res = DoCurrentVal();
            v = CurrentVal_;
            return res;
        }

    private:
        TConstIterator(const TTrie* t)
            : Trie(t)
        {
            if (Trie) {
                if (!Trie->Conf.ValueCodec) {
                    Begin = Trie->KeyVals.Begin();
                    End = Trie->KeyVals.End();
                } else {
                    BeginCoded = Trie->KeyValsCoded.Begin();
                    EndCoded = Trie->KeyValsCoded.End();
                }
            }
        }

        template <typename TTVal>
        bool DoCurrent(TStringBuf& k, TTVal& v, TBuffer* aux) const {
            if (!Trie || !DoCurrentKey() || !DoCurrentVal())
                return false;

            k = CurrentKey_;
            v = CurrentVal_;
            if (aux)
                aux->Assign(AuxBuffer.Data(), AuxBuffer.Size());
            return true;
        }

        template <typename TIter>
        bool DoNext(TIter& it, const TIter& begin, const TIter& end) {
            if (end == it)
                return false;

            if (it.IsEmpty()) {
                if (!AtEmptyValue && Trie->HasEmptyValue) {
                    AtEmptyValue = true;
                    return true;
                }

                AtEmptyValue = false;
                it = begin;
            } else {
                ++it;
            }

            return it != end;
        }

        bool AtEnd() const {
            bool coded = !!Trie->Conf.ValueCodec;

            return coded && (EndCoded == ItCoded || ItCoded.IsEmpty()) || !coded && (End == It || It.IsEmpty());
        }

        bool DoCurrentKey() const {
            if (BufferedKey_)
                return true;

            if (AtEmptyValue) {
                CurrentKey_.clear();
                BufferedKey_ = true;
                return true;
            }

            bool coded = !!Trie->Conf.ValueCodec;

            if (AtEnd())
                return false;

            CurrentKey_ = coded ? ItCoded.GetKey() : It.GetKey();

            if (!!Trie->Conf.KeyCodec) {
                Buffer.Clear();
                Trie->Conf.KeyCodec->Decode(CurrentKey_, Buffer);
                CurrentKey_.assign(Buffer.Data(), Buffer.Size());
            }

            BufferedKey_ = true;
            return true;
        }

        bool DoCurrentVal() const {
            if (BufferedVal_)
                return true;

            if (AtEmptyValue) {
                CurrentVal_ = Trie->EmptyValue;
                BufferedVal_ = true;
                return true;
            }

            bool coded = !!Trie->Conf.ValueCodec;

            if (AtEnd())
                return false;

            if (coded) {
                Buffer.Clear();
                Trie->Conf.ValueCodec->Decode(ItCoded.GetValue(), Buffer);
                TRepresentationFast r(&AuxBuffer);
                r.FromMemoryRegion(CurrentVal_, TStringBuf{Buffer.data(), Buffer.size()});
            } else {
                CurrentVal_ = It.GetValue();
            }

            BufferedVal_ = true;
            return true;
        }
    };

    template <typename TVal, typename TPacker>
    typename TCodecTrie<TVal, TPacker>::TConstIterator TCodecTrie<TVal, TPacker>::Iterator() const {
        return TConstIterator(this);
    }

}
