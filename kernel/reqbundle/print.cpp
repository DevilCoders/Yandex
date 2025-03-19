#include "print.h"

#include "serializer.h"
#include "block_pure.h"

#include <util/generic/xrange.h>

namespace {
    using namespace NReqBundle;

    const TPrintableSeparators& GetSeparators(bool isCustom) {
        static TPrintableSeparators defaultSeparators;
        return isCustom ? *Singleton<TPrintableSeparators>() : defaultSeparators;
    }

} // namespace

namespace NReqBundle {
    void SetCustomSeparators(const TPrintableSeparators& separators) {
        *Singleton<TPrintableSeparators>() = separators;
    }

    void TRequestPrinter::Init()
    {
        if (!Request.IsValid()) {
            return;
        }

        TReqBundleDeserializer deser;

        for (auto match : Request.GetMatches()) {
            Matches.insert({match.GetWordIndexFirst(), match});
            Types.insert(match.GetType());

            const size_t blockIndex = match.GetBlockIndex();
            if (Seq.IsValid() && !Blocks.contains(blockIndex)) {
                auto elem = Seq.GetElem(match);

                if (!elem.HasBlock()) {
                    TBlock* block = new TBlock;
                    deser.ParseBinary(elem.GetBinary(), *block);
                    AuxBlocks.push_back(block);
                    Blocks[blockIndex] = *block;
                } else {
                    Blocks[blockIndex] = elem.GetBlock();
                }
            }
        }
    }

    void TRequestPrinter::PrintWords(IOutputStream& out, EMatchType matchType, ui32 flags) const
    {
        const auto& sep = GetSeparators(flags & PF_CUSTOM_SEP);

        bool first = true;
        for (auto& entry : Matches) {
            auto match = entry.second;

            if (match.GetType() != matchType) {
                continue;
            }

            if (!first) {
                out << sep.MatchSeparator;
            }
            first = false;

            if (flags & PF_WORD_IDX) {
                if (match.GetWordIndexFirst() == match.GetWordIndexLast()) {
                    out << match.GetWordIndexFirst();
                } else {
                    out << sep.WordIndexLeftMargin
                        << match.GetWordIndexFirst()
                        << sep.WordIndexSeparator
                        << match.GetWordIndexLast()
                        << sep.WordIndexRightMargin;
                }
                out << sep.WordIndexBlockSeparator;
            }

            TConstBlockAcc block;

            if (Seq.IsValid()) {
                block = Blocks.find(match.GetBlockIndex())->second;
            }

            if (block.IsValid()) {
                if (flags & PF_BLOCK_IDX) {
                    out << match.GetBlockIndex() << sep.BlockIndexSeparator;
                }

                out << PrintableBlock(block, flags);
            } else {
                out << match.GetBlockIndex();
            }

            if (flags & (PF_WORD_FREQ | PF_MATCH_FREQ | PF_BLOCK_FREQ)) {
                out << sep.MatchFreqSeparator;
            }

            bool firstFreq = true;
            if (flags & PF_WORD_FREQ) {
                if (TMatch::OriginalWord == matchType) {
                    out << Request.GetWord(match.GetWordIndexFirst()).GetRevFreq();
                }
                firstFreq = false;
            }
            if (flags & PF_MATCH_FREQ) {
                out << (firstFreq ? "" : sep.FreqSeparator) << match.GetRevFreq();
                firstFreq = false;
            }
            if ((flags & PF_BLOCK_FREQ) && block.IsValid()) {
                out << (firstFreq ? "" : sep.FreqSeparator) << CalcCompoundRevFreq(block);
                firstFreq = false;
            }
        }
    }

    TString TRequestPrinter::GetBlockText(size_t blockIndex, ui32 flags) const
    {
        if (Seq.IsValid()) {
            return ToString(PrintableBlock(Blocks.find(blockIndex)->second, flags));
        }
        return TString();
    }

    bool TRequestPrinter::HasMatches(EMatchType matchType) const
    {
        return Types.contains(matchType);
    }

    void TRequestPrinter::SaveMatches(TMultiMap<size_t, TConstMatchAcc>& matches) const
    {
        matches = Matches;
    }
} // NReqBundle

template <>
void Out<NReqBundle::TBlock>(IOutputStream& out, const NReqBundle::TBlock& block)
{
    out << NReqBundle::PrintableBlock(block);
}

template <>
void Out<NReqBundle::TRequest>(IOutputStream& out, const NReqBundle::TRequest& request)
{
    out << NReqBundle::PrintableRequest(request);
}

template <>
void Out<NReqBundle::TReqBundle>(IOutputStream& out, const NReqBundle::TReqBundle& bundle)
{
    out << NReqBundle::PrintableReqBundle(bundle);
}

template <>
void Out<NReqBundle::TConstBlockAcc>(IOutputStream& out, const NReqBundle::TConstBlockAcc& block)
{
    out << NReqBundle::PrintableBlock(block);
}

template <>
void Out<NReqBundle::TConstRequestAcc>(IOutputStream& out, const NReqBundle::TConstRequestAcc& request)
{
    out << NReqBundle::PrintableRequest(request);
}

template <>
void Out<NReqBundle::TConstReqBundleAcc>(IOutputStream& out, const NReqBundle::TConstReqBundleAcc& bundle)
{
    out << NReqBundle::PrintableReqBundle(bundle);
}

template <>
void Out<NReqBundle::NDetail::TBlockOutputWrapper>(IOutputStream& out, const NReqBundle::NDetail::TBlockOutputWrapper& wrapper)
{
    const auto& sep = GetSeparators(wrapper.Flags & NReqBundle::PF_CUSTOM_SEP);

    if (wrapper.Block.IsWordBlock()) {
        out << wrapper.Block.GetWord(0).GetText();
    } else {
        out << sep.TextLeftMargin;
        for (size_t wordIndex : xrange(wrapper.Block.GetNumWords())) {
            if (wordIndex != 0) {
                out << sep.WordSeparator;
            }
            out << wrapper.Block.GetWord(wordIndex).GetText();
        }
        out << sep.TextRightMargin;
    }
}

template <>
void Out<NReqBundle::NDetail::TRequestOutputWrapper>(IOutputStream& out, const NReqBundle::NDetail::TRequestOutputWrapper& wrapper)
{
    if (!wrapper.Request.IsValid()) {
        return;
    }

    const auto& sep = GetSeparators(wrapper.Flags & PF_CUSTOM_SEP);

    NReqBundle::TRequestPrinter printer(wrapper.Request, wrapper.Sequence);

    printer.PrintWords(out, NLingBoost::TMatch::OriginalWord, wrapper.Flags);

    if ((wrapper.Flags & NReqBundle::PF_SYNONYMS) && printer.HasMatches(NLingBoost::TMatch::Synonym)) {
        out << sep.SynonymsSeparator;
        printer.PrintWords(out, NLingBoost::TMatch::Synonym, wrapper.Flags);
    }

    if (wrapper.Flags & (NReqBundle::PF_FACET_NAME | NReqBundle::PF_FACET_VALUE)) {
        out << sep.RequestFacetSeparator;
        out << sep.FacetLeftMargin;

        bool firstEntry = true;
        for (auto entry : wrapper.Request.GetFacets().GetEntries()) {
            if (!NStructuredId::IsSubId(entry.GetId(), wrapper.Id)) {
                continue;
            }

            if (!firstEntry) {
                out << sep.FacetSeparator;
            }
            firstEntry = false;
            if (wrapper.Flags & NReqBundle::PF_FACET_NAME) {
                out << entry.GetId().FullName();
                if (wrapper.Flags & NReqBundle::PF_FACET_VALUE) {
                    out << sep.FacetValueSeparator;
                }
            }
            if (wrapper.Flags & NReqBundle::PF_FACET_VALUE) {
                out << entry.GetValue();
            }
        }

        out << sep.FacetRightMargin;
    }
}

template <>
void Out<NReqBundle::NDetail::TReqBundleOutputWrapper>(IOutputStream& out, const NReqBundle::NDetail::TReqBundleOutputWrapper& wrapper)
{
    if (!wrapper.Bundle.IsValid()) {
        return;
    }

    const auto& sep = GetSeparators(wrapper.Flags & PF_CUSTOM_SEP);

    bool firstRequest = true;
    for (size_t index : xrange(wrapper.Bundle.GetNumRequests())) {
        auto request = wrapper.Bundle.GetRequest(index);

        bool accept = (NReqBundle::TFacetId() == wrapper.Id);
        if (!accept) {
            for (auto entry : request.GetFacets().GetEntries()) {
                if (NStructuredId::IsSubId(entry.GetId(), wrapper.Id)) {
                    accept = true;
                    break;
                }
            }
        }
        if (!accept) {
            continue;
        }

        if (!firstRequest) {
            out << sep.RequestSeparator;
        }
        firstRequest = false;

        if (wrapper.Flags & NReqBundle::PF_REQUEST_IDX) {
            out << index << sep.RequestIndexSeparator;
        }
        out << NReqBundle::PrintableRequest(wrapper.Bundle, index, wrapper.Id, wrapper.Flags);
    }
}
