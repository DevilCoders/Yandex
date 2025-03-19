#include "dict_builder.h"
#include "grammar.h"
#include "common.h"

#include <library/cpp/getopt/last_getopt.h>
#include <util/folder/filelist.h>
#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

using namespace NNewLemmer;

namespace NDictBuild {
    TNewProtoDictBuilder::TNewProtoDictBuilder(
        const TVector<TString>& grammar,
        const TGramIds& grammarAddrs,
        const TDataStatistics& statistics, //Flexies, Schemes, Blocks
        const TVector<TString>& flexTries,
        const TString& endsTrie,
        const TVector<TSinglePackedEnd>& uniqueEnds,
        const TVector<TVector<size_t>>& anasLists,
        const TDataFreq& freqs,
        const TDictBuildParams& params) {
        //TString path = "/place/home/mihaild/trunk/arcadia.MORPH-82/dict/tools/make_morphdict/rewrite_tests/python/eng..--lcs";
        //Grammars
        {
            for (const auto& g : grammar) {
                TUtf16String line = UTF8ToWide(g);
                TString res;
                if (params.DetachGrammar)
                    DetachGrammar(line, res, params.Verbose);
                else
                    res = WideToUTF8(line);
                res.push_back(0);
                Dict.AddGrammars(res);
            }
        }

        //GrammarIndexes
        {
            for (const auto& g : grammarAddrs.GetAddrs()) {
                for (const auto& endId : g.first) {
                    NLemmerProto::TRef& grammarRef = *Dict.AddGrammarRefs();
                    grammarRef.SetId(endId);
                    grammarRef.SetHasNext(true);
                }
                ui32 endId = g.second;
                NLemmerProto::TRef& grammarRef = *Dict.AddGrammarRefs();
                grammarRef.SetId(endId);
                grammarRef.SetHasNext(false);
            }
        }

        // Flexies
        {
            size_t i = 0;
            using TFlexWithId = std::pair<TUtf16String, size_t>;
            TVector<TFlexWithId> allFlexies;
            for (auto line : SortMapByValue(statistics.Flexies)) {
                Reverse(line.begin(), line.begin() + line.size());
                allFlexies.push_back(TFlexWithId{line, i});
                ++i;
            }
            Sort(
                allFlexies.begin(),
                allFlexies.end(),
                [](const TFlexWithId& x, const TFlexWithId& y) { return x.first > y.first || (x.first == y.first && x.second < y.second); });
            for (size_t j = 0; j != allFlexies.size(); ++j) {
                auto& flex = allFlexies[j].first;
                auto id = allFlexies[j].second;
                Reverse(flex.begin(), flex.begin() + flex.size());
                flex.push_back(0);
                FlexiesText[id] = flex;
            }
        }

        // FlexTries
        {
            size_t schemesCount = flexTries.size();
            for (size_t sch = 0; sch < flexTries.size(); ++sch) {
                Stems[sch] = flexTries[sch];
            }
            assert(schemesCount == Stems.size());
            FlexTriesAddrs.resize(schemesCount);
            if (schemesCount > 0) {
                FlexTriesAddrs[0] = 0;
                TVector<bool> saved(schemesCount, false);
                for (size_t sch = 0; sch != schemesCount; ++sch) {
                    if (sch > 0)
                        FlexTriesAddrs[sch] = FlexTriesAddrs[sch - 1] + Stems[sch - 1].size();
                    NLemmerProto::TTrie& trie = *Dict.AddFlexTries();
                    trie.SetBinary(Stems[sch]);
                }
            }
        }

        // PatternsTrie
        {
            NLemmerProto::TTrie& trie = *Dict.MutablePatternsTrie();
            trie.SetBinary(endsTrie);
        }

        TVector<size_t> frequencies;
        {
            if (params.UseFreqs) {
                size_t i = 0;
                frequencies.resize(freqs.AllFreqValues.size());
                for (ui32 val : freqs.AllFreqValues) {
                    frequencies[i] = val;
                    i++;
                }
            }
        }
        // Schemes
        {
            for (const auto& scheme : statistics.Schemes) {
                NLemmerProto::TScheme& s = *Dict.AddSchemes();
                NLemmerProto::TFlexie& flex = *s.MutableFlex();
                flex.SetText(reinterpret_cast<const char*>(FlexiesText[scheme.FlexId].c_str()), FlexiesText[scheme.FlexId].size() * sizeof(wchar16));
                s.SetGrammarId(Dict.GetGrammarRefs(scheme.GramId).GetId());
                s.SetBlockId(scheme.BlockId);
                if (params.UseFreqs) {
                    s.SetFrequency(frequencies[scheme.FreqId]);
                }
            }
        }

        // Blocks
        {
            for (const auto& block : statistics.Blocks) {
                NLemmerProto::TBlock& b = *Dict.AddBlocks();
                if (params.UseGeneration) {
                    NLemmerProto::TFlexie& flex = *b.MutableFlex();
                    flex.SetText(reinterpret_cast<const char*>(FlexiesText[block.FlexId].c_str()), FlexiesText[block.FlexId].size() * sizeof(wchar16));
                    b.SetHasNext(block.HasNext);
                }
                b.SetGrammarRefIndex(block.GramId);
                if (params.UseFreqs) {
                    b.SetFrequency(frequencies[block.FreqId]);
                }
            }
        }

        // Patterns
        {
            for (const auto& end : uniqueEnds) {
                auto& e = *Dict.AddPatterns();
                auto packed = end.EndInfo.Pack();
                e.SetBitmask(packed);
                if (params.UseFreqs) {
                    ui16 freqId = end.Freq;
                    e.SetFrequency(frequencies[freqId]);
                }
                if (params.UseDiaMask) {
                    ui16 diaMask = end.Mask;
                    e.SetDiaMask(diaMask);
                }
            }
        }

        // MatchedPatterns
        {

            for (const auto& l : anasLists) {
                for (size_t i = 0; i + 1 < l.size(); ++i) {
                    ui32 val = l[i];
                    NLemmerProto::TRef& matchedPattern = *Dict.AddMatchedPatterns();
                    matchedPattern.SetId(val);
                    matchedPattern.SetHasNext(true);
                }
                ui32 val = l.back();
                NLemmerProto::TRef& matchedPattern = *Dict.AddMatchedPatterns();
                matchedPattern.SetId(val);
                matchedPattern.SetHasNext(false);
            }
        }

        // Fingerprint
        if (!params.Fingerprint.empty()) {
            Dict.SetFingerprint(params.Fingerprint);
        }
    }

    void TNewProtoDictBuilder::Save(IOutputStream& out) const {
        Dict.SerializeToArcadiaStream(&out);
    }

    TNewBinaryDictBuilder::TNewBinaryDictBuilder(
        const TVector<TString>& grammar,
        const TGramIds& grammarAddrs,
        const TDataStatistics& statistics, //Flexies, Schemes, Blocks
        const TVector<TString>& flexTries,
        const TString& endsTrie,
        const TVector<TSinglePackedEnd>& uniqueEnds,
        const TVector<TVector<size_t>>& anasLists,
        const TDataFreq& freqs,
        const TDictBuildParams& params) {
        //TString path = "/place/home/mihaild/trunk/arcadia.MORPH-82/dict/tools/make_morphdict/rewrite_tests/python/eng..--lcs";

        Header.Init();
        size_t size = 0;
        // Grammar
        {
            Header.Offset[F_GRAMMAR] = sizeof(THeader) + Buffer.Size();
            size_t counter = 0;
            for (const auto& g : grammar) {
                TUtf16String line = UTF8ToWide(g);
                TString res;
                if (params.DetachGrammar)
                    DetachGrammar(line, res, params.Verbose);
                else
                    res = WideToUTF8(line);
                res.push_back(0);
                GrammarAddrs[counter++] = Buffer.Size();
                Buffer.Append(res.c_str(), res.size());
            }
            Header.Length[F_GRAMMAR] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_GRAMMAR];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "Grammar:     " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        //GrammIds
        {
            Header.Offset[F_GRAMMAR_ADDRS] = sizeof(THeader) + Buffer.Size();
            for (const auto& g : grammarAddrs.GetAddrs()) {
                for (const auto& endId : g.first) {
                    ui32 val = GrammarAddrs[endId] | (1 << 31);
                    Buffer.Append(reinterpret_cast<const char*>(&val), BYTES[F_GRAMMAR_ADDRS]);
                }
                ui32 endId = g.second;
                ui32 val = GrammarAddrs[endId];
                Buffer.Append(reinterpret_cast<const char*>(&val), BYTES[F_GRAMMAR_ADDRS]);
            }
            Header.Length[F_GRAMMAR_ADDRS] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_GRAMMAR_ADDRS];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "GrammarIds:  " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // Flexies
        {
            Header.Offset[F_FLEXIES] = sizeof(THeader) + Buffer.Size();
            size_t i = 0;
            using TFlexWithId = std::pair<TUtf16String, size_t>;
            TVector<TFlexWithId> allFlexies;
            for (auto line : SortMapByValue(statistics.Flexies)) {
                Reverse(line.begin(), line.begin() + line.size());
                allFlexies.push_back(TFlexWithId{line, i});
                ++i;
            }
            Sort(
                allFlexies.begin(),
                allFlexies.end(),
                [](const TFlexWithId& x, const TFlexWithId& y) { return x.first > y.first || (x.first == y.first && x.second < y.second); });
            TUtf16String flexiesString;
            for (size_t j = 0; j != allFlexies.size(); ++j) {
                auto& flex = allFlexies[j].first;
                auto id = allFlexies[j].second;
                Reverse(flex.begin(), flex.begin() + flex.size());
                flex.push_back(0);
                if (j > 0 && allFlexies[j - 1].first.EndsWith(flex)) {
                    const auto& prevFlex = allFlexies[j - 1].first;
                    auto prevId = allFlexies[j - 1].second;
                    Flexies[id] = Flexies[prevId] + (prevFlex.size() - flex.size()) * sizeof(wchar16);
                } else {
                    Flexies[id] = Buffer.Size() - size;
                    Buffer.Append(reinterpret_cast<const char*>(flex.c_str()), flex.size() * sizeof(wchar16));
                    flexiesString += flex;
                }
            }
            Header.Length[F_FLEXIES] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_FLEXIES];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "Flexies:     " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // FlexiesList
        {
            Header.Offset[F_FLEXIES_ADDRS] = sizeof(THeader) + Buffer.Size();
            for (size_t i = 0; i != Flexies.size(); ++i) {
                Buffer.Append(reinterpret_cast<const char*>(&Flexies[i]), BYTES[F_FLEXIES_ADDRS]);
            }
            Header.Length[F_FLEXIES_ADDRS] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_FLEXIES_ADDRS];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "FlexiesList: " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // FlexTries
        {
            Header.Offset[F_FLEX_TRIES] = sizeof(THeader) + Buffer.Size();
            size_t schemesCount = flexTries.size();
            for (size_t sch = 0; sch < flexTries.size(); ++sch) {
                Stems[sch] = flexTries[sch];
            }
            assert(schemesCount == Stems.size());
            FlexTriesAddrs.resize(schemesCount);

            if (schemesCount > 0) {
                FlexTriesAddrs[0] = 0;
                TVector<bool> saved(schemesCount, false);
                for (size_t sch = 0; sch != schemesCount; ++sch) {
                    if (sch > 0)
                        FlexTriesAddrs[sch] = FlexTriesAddrs[sch - 1] + Stems[sch - 1].size();
                    Buffer.Append(Stems[sch].c_str(), Stems[sch].size());
                }
                Buffer.AlignUp(4);
            }
            Header.Length[F_FLEX_TRIES] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_FLEX_TRIES];
        }
        if (params.Verbose) {
            Cerr << "SchemesCount:" << FlexTriesAddrs.size() << Endl;
            Cerr << "FlexTries:   " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // FlexTriesAddrs
        {
            Header.Offset[F_FLEX_TRIES_ADDRS] = sizeof(THeader) + Buffer.Size();
            for (size_t sch = 0; sch != FlexTriesAddrs.size(); ++sch) {
                ui32 val = FlexTriesAddrs[sch];
                Buffer.Append(reinterpret_cast<const char*>(&val), BYTES[F_FLEX_TRIES_ADDRS]);
            }
            Header.Length[F_FLEX_TRIES_ADDRS] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_FLEX_TRIES_ADDRS];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "FTriesAddrs: " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // Ends
        {
            Header.Offset[F_ENDS_TRIE] = sizeof(THeader) + Buffer.Size();
            //TFileInput in(path + "/ends.trie");
            TString ends = endsTrie;
            Buffer.Append(ends.c_str(), ends.size());
            Header.Length[F_ENDS_TRIE] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_ENDS_TRIE];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "Ends:        " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // Schemes_LemmaFlex
        {
            Header.Offset[F_SCHEMES_LEMMA_FLEX] = sizeof(THeader) + Buffer.Size();
            /*TFileInput in(path + "/schemes.lemmaFlex");
            TString line;
            while (in.ReadLine(line)) {
                ui32 lemmaFlexId = FromString<ui32>(line);*/
            for (const auto& scheme : statistics.Schemes) {
                ui32 lemmaFlexId = scheme.FlexId;
                Buffer.Append(reinterpret_cast<const char*>(&lemmaFlexId), BYTES[F_SCHEMES_LEMMA_FLEX]);
            }
            Header.Length[F_SCHEMES_LEMMA_FLEX] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_SCHEMES_LEMMA_FLEX];
            Buffer.AlignUp(4);
        }

        // Schemes_StemGram
        {
            Header.Offset[F_SCHEMES_STEM_GRAM] = sizeof(THeader) + Buffer.Size();
            /*TFileInput in(path + "/schemes.stemGram");
            TString line;
            while (in.ReadLine(line)) {
            ui16 stemGramId = FromString<ui16>(line);*/
            for (const auto& scheme : statistics.Schemes) {
                ui32 stemGramId = scheme.GramId;
                Buffer.Append(reinterpret_cast<const char*>(&stemGramId), BYTES[F_SCHEMES_STEM_GRAM]);
            }
            Header.Length[F_SCHEMES_STEM_GRAM] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_SCHEMES_STEM_GRAM];
            Buffer.AlignUp(4);
        }

        // Schemes_BlockId
        {
            Header.Offset[F_SCHEMES_BLOCK_ID] = sizeof(THeader) + Buffer.Size();
            /*TFileInput in(path + "/schemes.blockId");
            TString line;
            while (in.ReadLine(line)) {
                ui32 blockId = FromString<ui32>(line);*/
            for (const auto& scheme : statistics.Schemes) {
                ui32 blockId = scheme.BlockId;
                Buffer.Append(reinterpret_cast<const char*>(&blockId), BYTES[F_SCHEMES_BLOCK_ID]);
            }
            Header.Length[F_SCHEMES_BLOCK_ID] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_SCHEMES_BLOCK_ID];
            Buffer.AlignUp(4);
        }

        // Schemes_FreqId
        if (params.UseFreqs) {
            Header.Offset[F_SCHEMES_FREQ_ID] = sizeof(THeader) + Buffer.Size();
            /*TFileInput in(path + "/schemes.freqId");
            TString line;
            while (in.ReadLine(line)) {
                ui32 freq = FromString<ui32>(line);*/
            for (const auto& scheme : statistics.Schemes) {
                ui32 freq = scheme.FreqId;
                Buffer.Append(reinterpret_cast<const char*>(&freq), BYTES[F_SCHEMES_FREQ_ID]);
            }
            Header.Length[F_SCHEMES_FREQ_ID] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_SCHEMES_FREQ_ID];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "Schemes:     " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // Blocks_FormFlex
        if (params.UseGeneration) {
            Header.Offset[F_BLOCKS_FORM_FLEX] = sizeof(THeader) + Buffer.Size();
            for (const auto& block : statistics.Blocks) {
                ui32 flexId = block.FlexId;
                bool hasNext = block.HasNext;
                flexId |= (ui32(hasNext) << 31);
                Buffer.Append(reinterpret_cast<const char*>(&flexId), BYTES[F_BLOCKS_FORM_FLEX]);
            }
            Header.Length[F_BLOCKS_FORM_FLEX] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_BLOCKS_FORM_FLEX];
            Buffer.AlignUp(4);
        }

        // Blocks_FlexGram
        {
            Header.Offset[F_BLOCKS_FLEX_GRAM] = sizeof(THeader) + Buffer.Size();
            for (const auto& block : statistics.Blocks) {
                ui16 gramId = block.GramId;
                Buffer.Append(reinterpret_cast<const char*>(&gramId), BYTES[F_BLOCKS_FLEX_GRAM]);
            }
            Header.Length[F_BLOCKS_FLEX_GRAM] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_BLOCKS_FLEX_GRAM];
            Buffer.AlignUp(4);
        }

        // Blocks_FreqId
        if (params.UseFreqs) {
            Header.Offset[F_BLOCKS_FREQ_ID] = sizeof(THeader) + Buffer.Size();
            for (const auto& block : statistics.Blocks) {
                ui32 freq = block.FreqId;
                Buffer.Append(reinterpret_cast<const char*>(&freq), BYTES[F_BLOCKS_FREQ_ID]);
            }
            Header.Length[F_BLOCKS_FREQ_ID] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_BLOCKS_FREQ_ID];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "Blocks:      " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // EndInfo
        {
            Header.Offset[F_END_INFO] = sizeof(THeader) + Buffer.Size();
            for (const auto& end : uniqueEnds) {
                auto packed = end.EndInfo.Pack();
                Buffer.Append(reinterpret_cast<const char*>(&packed), 4);
            }
            Header.Length[F_END_INFO] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_END_INFO];
            Buffer.AlignUp(4);
        }

        // End_FreqId
        if (params.UseFreqs) {
            Header.Offset[F_END_FREQ_ID] = sizeof(THeader) + Buffer.Size();
            for (const auto& end : uniqueEnds) {
                ui16 freqId = end.Freq;
                Buffer.Append(reinterpret_cast<const char*>(&freqId), BYTES[F_END_FREQ_ID]);
            }
            Header.Length[F_END_FREQ_ID] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_END_FREQ_ID];
            Buffer.AlignUp(4);
        }

        // End_DiaMask
        if (params.UseDiaMask) {
            Header.Offset[F_END_DIA_MASK] = sizeof(THeader) + Buffer.Size();
            for (const auto& end : uniqueEnds) {
                ui16 diaMask = end.Mask;
                Buffer.Append(reinterpret_cast<const char*>(&diaMask), BYTES[F_END_DIA_MASK]);
            }
            Header.Length[F_END_DIA_MASK] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_END_DIA_MASK];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "Anas:        " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // AnasLists
        {
            Header.Offset[F_ANAS_LISTS] = sizeof(THeader) + Buffer.Size();
            for (const auto& l : anasLists) {
                for (size_t i = 0; i + 1 < l.size(); ++i) {
                    ui32 val = l[i];
                    val |= (1 << 31);
                    Buffer.Append(reinterpret_cast<const char*>(&val), BYTES[F_ANAS_LISTS]);
                }
                ui32 val = l.back();
                Buffer.Append(reinterpret_cast<const char*>(&val), BYTES[F_ANAS_LISTS]);
            }
            Header.Length[F_ANAS_LISTS] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_ANAS_LISTS];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "AnasLists:   " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // Freqs
        if (params.UseFreqs) {
            Header.Offset[F_FREQS] = sizeof(THeader) + Buffer.Size();
            /*TFileInput in(path + "/freqs");
            TString line;
            while (in.ReadLine(line)) {
                ui32 val = FromString<ui32>(line);*/
            for (ui32 val : freqs.AllFreqValues) {
                Buffer.Append(reinterpret_cast<const char*>(&val), BYTES[F_FREQS]);
            }
            Header.Length[F_FREQS] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_FREQS];
            Buffer.AlignUp(4);
        }
        if (params.Verbose) {
            Cerr << "Freqs:       " << Buffer.Size() - size << Endl;
        }
        size = Buffer.Size();

        // Fingerprint
        if (!params.Fingerprint.empty()) {
            Header.Offset[F_FINGERPRINT] = sizeof(THeader) + Buffer.Size();
            Buffer.Append(params.Fingerprint.c_str(), params.Fingerprint.size());
            Header.Length[F_FINGERPRINT] = sizeof(THeader) + Buffer.Size() - Header.Offset[F_FINGERPRINT];
        }
        if (params.Verbose) {
            Cerr << "Fingerprint: " << Buffer.Size() - size << Endl;
        }
    }

    void TNewBinaryDictBuilder::Save(IOutputStream& out) const {
        TBuffer headerBuffer;
        headerBuffer.Append(reinterpret_cast<const char*>(&Header), sizeof(Header));
        headerBuffer.AlignUp(4);
        out.Write(headerBuffer.Data(), headerBuffer.Size());
        out.Write(Buffer.Data(), Buffer.Size());
    }
}

