#pragma once

#include "mine_address.h"
#include "options.h"

#include <kernel/remorph/text/textprocessor.h>
#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/common/article_util.h>


class TAddressExtractor {

public:
    TAddressExtractor(const TExtractorOpts& opts, IOutputStream* output, const TString& protoName, const NGzt::TGazetteer* externalGzt = nullptr)
        : Options(opts)
        , OutStreamPtr(output)
        , TokenizeOpts(NToken::TF_NO_SENTENCE_SPLIT, NToken::BD_NONE, NToken::MS_MINIMAL)
        , currentFactCount(0)
        , PreparedTextLength(0)
        , CurrentBlockTextLength(0)
    {
        TextFactProcessor.Reset(new NText::TTextFactProcessor(protoName, externalGzt));
    }

    TAddressExtractor(TAtomicSharedPtr<NText::TTextFactProcessor> factProcessor,
                      const TExtractorOpts& opts,
                      IOutputStream* output)
        : TextFactProcessor(factProcessor)
        , Options(opts)
        , OutStreamPtr(output)
        , TokenizeOpts(NToken::TF_NO_SENTENCE_SPLIT, NToken::BD_NONE, NToken::MS_MINIMAL)
        , currentFactCount(0)
        , PreparedTextLength(0)
        , CurrentBlockTextLength(0)
    {
    }

    void ProcessLine(const TUtf16String& wText) {
        if (CurrentBlockResult.size() > 0) {
            DocResult.push_back(CurrentBlockResult);
            CurrentBlockResult.clear();
        }

        PreparedTextLength += CurrentBlockTextLength;

        TextFactProcessor->ProcessText(*this, wText, TokenizeOpts, Options.Lang);

        CurrentBlockTextLength = wText.length();
    }

    void ProcessLine(const TString& text) {
        TUtf16String wText;
        NDetail::Recode<char>(text, wText, Options.Encoding);
        ProcessLine(wText);
    }

    void ProcessEnd() {
        CombineFacts();

        if (Options.DoAutoPrintResult) {
            PrintResult();
        }

        Reset();
    }

    TVector<TMineAddressPtr> GetFacts() {
        CombineFacts();
        TVector<TMineAddressPtr> result = GetFilteredFacts();

        return result;
    }

    void ResetOutputStream(IOutputStream* output) {
        OutStreamPtr = output;
    }

    void operator() (const NToken::TSentenceInfo& sentence, const NText::TWordSymbols& words, const TVector<NFact::TFactPtr>& facts);

private:

    void PrintSpaces(int n) const;
    void PrintField(NFact::TCompoundFieldValuePtr field, int depth) const;
    void PrintField(NFact::TFieldValuePtr field, int depth) const;
    void PrintFact(NFact::TFactPtr fact, const NText::TWordSymbols& words) const;


    //Trying to combine every fact to next one in line. If successfull take first good variant
    void CombineToNextInLine() {
        for(size_t i = 0; i < DocResult.size(); i++) {
            for(int j = 0; j < DocResult[i].ysize() - 1; j++) {
                TVector<TMineAddressPtr> result = DocResult[i][j + 1]->CombineWithGeoPart(DocResult[i][j]);
                if (result.size() > 0) {
                    DocResult[i][j + 1] = result[0];
                }
            }
        }
    }

    //Trying to combine every fact to next one in text, not only in line. If successfull take first good variant
    void CombineToNext() {
        TMineAddressPtr prevFact;
        for(size_t i = 0; i < DocResult.size(); i++) {
            for(int j = 0; j < DocResult[i].ysize(); j++) {
                if (!!prevFact) {
                    TVector<TMineAddressPtr> result = DocResult[i][j]->CombineWithGeoPart(prevFact);
                    if (result.size() > 0) {
                        DocResult[i][j] = result[0];
                    }
                }
                prevFact = DocResult[i][j];
            }
        }
    }


    // Trying to handle with
    // Country:
    //    City:
    //       Address
    //       Address
    //    City:
    //       Address
    //       Address
    //    City:
    //       Address
    //       Address
    void CombineWithSectionHeader();

    //Find first geo facts (one for every geo level) that can be added to every other fact in document
    //And combine them into every fact in order from village to country
    void CombineWithSingleGeo();

    void Reset() {
        CurrentBlockResult.clear();
        DocResult.clear();
        currentFactCount = 0;
        PreparedTextLength = 0;
        CurrentBlockTextLength = 0;
    }

    void CombineFacts() {
        if (CurrentBlockResult.size() > 0) {
            DocResult.push_back(CurrentBlockResult);
            CurrentBlockResult.clear();
        }

        if (Options.CombineWithHeader)
            CombineWithSectionHeader();

        if (Options.CombineWithNextInLine)
            CombineToNextInLine();

        if (Options.CombineWithNext)
            CombineToNext();

        if (Options.CombineWithTheOnlyCity)
            CombineWithSingleGeo();

    }

    TVector<TMineAddressPtr> GetFilteredFacts() const;

    void PrintResult() const;

private:
    TAtomicSharedPtr<NText::TTextFactProcessor> TextFactProcessor;
    const TExtractorOpts& Options;
    IOutputStream* OutStreamPtr;
    NToken::TTokenizeOptions TokenizeOpts;
    TVector<TMineAddressPtr> CurrentBlockResult;
    TVector< TVector<TMineAddressPtr> > DocResult;

    size_t currentFactCount;
    size_t PreparedTextLength;
    size_t CurrentBlockTextLength;
};
