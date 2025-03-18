#pragma once

#include "memory_representation.h"
#include "raw_pair.h"
#include "trie_conf.h"

#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/containers/paged_vector/paged_vector.h>

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <utility>
#include <functional>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>
#include <util/stream/format.h>
#include <util/system/rusage.h>

#include <algorithm>

namespace NCodecTrie {
    template <typename TVal = TString, typename TPacker = TCompactTriePacker<TVal>>
    class TCodecTrieBuilder {
        using TRepresentationFast = TMemoryRepresentation<TVal, TPacker, false>;
        using TRepresentationSafe = TMemoryRepresentation<TVal, TPacker, true>;
        using TKeyValPairs = NPagedVector::TPagedVector<TRawPair>;
        using TKeyValsTrie = TCompactTrie<char, TVal, TPacker>;
        using TKeyValsCodedTrie = TCompactTrie<char, TStringBuf>;

        TCodecTrieConf Conf;

        TBuffer EmptyValueBuffer;
        ui8 HasEmptyValue = 0;

        TKeyValPairs RawPairs;

        TBuffer ConvBuf;

    public:
        explicit TCodecTrieBuilder(const TCodecTrieConf& conf = TCodecTrieConf())
            : Conf(conf)
        {
        }

        void Add(TStringBuf key, const TVal& value);

        TBlob Compact(); // destroys data in builder

    private:
        static TStringBuf GetKeyFromPair(TKeyValPairs::const_iterator it) {
            return it->GetKey();
        }

        static TStringBuf GetValueFromPair(TKeyValPairs::const_iterator it) {
            return it->GetVal();
        }

        static bool GreaterOfPairs(const TRawPair& a, const TRawPair& b) {
            return a.GetKey() > b.GetKey();
        };

        void PostProcessTrie(TBuffer& trie, size_t offset, std::function<void(IOutputStream&, const char*, size_t)> proc) {
            TBufferOutput bout(trie.Size() * 1.1);
            bout.Write(trie.Data(), offset);
            proc(bout, trie.data() + offset, trie.size() - offset);
            trie.Swap(bout.Buffer());
        }

        template <class TBuilder, class TPckr>
        TBlob Serialize(TBuilder& builder, ui64 sz) {
            TBuffer result(sz);
            TBufferOutput bout(result);

            Conf.Save(&bout);
            ::Save(&bout, HasEmptyValue);

            if (HasEmptyValue) {
                ::Save(&bout, (ui64)EmptyValueBuffer.Size());
                bout.Write(EmptyValueBuffer.data(), EmptyValueBuffer.size());
            }

            // TODO: can we serialize without memcpying the trie output?
            {
                PrintStatusMessage("Serializing compact trie...");
                size_t headersize = bout.Buffer().Size();
                builder.Save(bout);
                builder.Clear();
                bout.Buffer().ShrinkToFit();

                if (Conf.Minimize) {
                    PostProcessTrie(result, headersize, [&](IOutputStream& out, const char* begin, size_t szsz) {
                        CompactTrieMinimize(out, begin, szsz, Conf.Verbose, TPckr());
                    });
                }

                if (Conf.MakeFastLayout) {
                    PostProcessTrie(result, headersize, [&](IOutputStream& out, const char* begin, size_t szsz) {
                        CompactTrieMakeFastLayout(out, begin, szsz, Conf.Verbose, TPckr());
                    });
                }

                PrintStatusMessage("Done");
            }

            return TBlob::FromBuffer(result);
        }

        void PrintStatusMessage(const char* mess) const {
            if (Conf.Verbose) {
                Clog << RightPad(mess, 80, ' ') << " RSS=" << (TRusage::Get().MaxRss >> 20) << "M" << Endl;
            }
        }
    };

    template <typename TVal, typename TPacker>
    void TCodecTrieBuilder<TVal, TPacker>::Add(TStringBuf key, const TVal& value) {
        TRepresentationFast rep(&ConvBuf);
        if (key.empty()) {
            TStringBuf mr = rep.ToMemoryRegion(value);
            EmptyValueBuffer.Assign(mr.data(), mr.size());
            HasEmptyValue = true;
        } else {
            RawPairs.push_back(TRawPair(key, rep.ToMemoryRegion(value)));
        }
    }

    template <typename TVal, typename TPacker>
    TBlob TCodecTrieBuilder<TVal, TPacker>::Compact() {
        Conf.ResetCodecs();

        if (!!Conf.KeyCodec && Conf.KeyCodec->Traits().NeedsTraining) {
            PrintStatusMessage("Training key codec...");
            Conf.KeyCodec->Learn(RawPairs.begin(), RawPairs.end(), GetKeyFromPair);
            PrintStatusMessage("Done");
        }

        if (!!Conf.ValueCodec && Conf.ValueCodec->Traits().NeedsTraining) {
            PrintStatusMessage("Training value codec...");
            Conf.ValueCodec->Learn(RawPairs.begin(), RawPairs.end(), GetValueFromPair);
            PrintStatusMessage("Done");
        }

        PrintStatusMessage("Encoding keys and vals...");
        ui64 size = 0;
        TBuffer k;
        TBuffer v;
        for (typename TKeyValPairs::iterator it = RawPairs.begin(); it != RawPairs.end(); ++it) {
            k.Clear();
            v.Clear();

            if (!Conf.KeyCodec)
                k.Assign(it->GetKey().begin(), it->GetKey().end());
            else
                Conf.KeyCodec->Encode(it->GetKey(), k);

            if (!Conf.ValueCodec)
                v.Assign(it->GetVal().begin(), it->GetVal().end());
            else
                Conf.ValueCodec->Encode(it->GetVal(), v);

            size += k.Size() + v.Size();
            it->Assign(TStringBuf{k.data(), k.size()}, TStringBuf{v.data(), v.size()});
        }

        PrintStatusMessage("Done");
        PrintStatusMessage("Sorting...");

        if (!Conf.KeyCodec && Conf.Sorted) {
            RawPairs.resize(Unique(RawPairs.begin(), RawPairs.end()) - RawPairs.begin());
            std::reverse(RawPairs.begin(), RawPairs.end());
        } else {
            Sort(RawPairs.begin(), RawPairs.end(), GreaterOfPairs);
            RawPairs.resize(Unique(RawPairs.begin(), RawPairs.end()) - RawPairs.begin());
        }

        PrintStatusMessage("Done");
        PrintStatusMessage("Assembling compact trie...");

        TBlob res;

        if (!Conf.ValueCodec) {
            typedef typename TKeyValsTrie::TBuilder TBuilder;
            TBuilder builder(CTBF_PREFIX_GROUPED | (Conf.Verbose ? CTBF_VERBOSE : CTBF_NONE));

            for (typename TKeyValPairs::reverse_iterator it = RawPairs.rbegin(); it != RawPairs.rend(); it = RawPairs.rbegin()) {
                ConvBuf.Clear();
                TRepresentationFast rep(&ConvBuf);
                TVal vv;
                rep.FromMemoryRegion(vv, it->GetVal());
                builder.Add(it->GetKey().data(), it->GetKey().size(), vv);
                RawPairs.pop_back();
            }

            res = Serialize<TBuilder, typename TBuilder::TPacker>(builder, size);
        } else {
            typedef TKeyValsCodedTrie::TBuilder TBuilder;
            TBuilder builder(CTBF_PREFIX_GROUPED | (Conf.Verbose ? CTBF_VERBOSE : CTBF_NONE));

            for (typename TKeyValPairs::reverse_iterator it = RawPairs.rbegin(); it != RawPairs.rend(); it = RawPairs.rbegin()) {
                builder.Add(it->GetKey().data(), it->GetKey().size(), it->GetVal());
                RawPairs.pop_back();
            }

            res = Serialize<TBuilder, TBuilder::TPacker>(builder, size);
        }

        PrintStatusMessage("Done");

        return res;
    }

}
