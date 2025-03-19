#pragma once

#include <kernel/lingboost/freq.h>
#include <kernel/qtree/request/nodebase.h>
#include <kernel/qtree/richrequest/richnode_fwd.h>

#include <kernel/lemmer/core/lemmaforms.h>
#include <library/cpp/token/serialization/protos/char_span.pb.h>
#include <library/cpp/token/nlptypes.h>
#include <library/cpp/token/token_structure.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/langs/langs.h>

#include <util/charset/wide.h>
#include <util/memory/blob.h>

class TProximity;
class TWordNode;

namespace NReqBundle {
namespace NDetail {
    class TText {
    private:
        mutable TString Text;
        mutable TUtf16String WideText;

    public:
        TText() = default;

        TText(const TString& text)
            : Text(text)
        {}

        TText(const TUtf16String& wideText)
            : WideText(wideText)
        {}

        TText& operator = (const TString& text) {
            Text = text;
            WideText = TUtf16String();
            return *this;
        }

        TText& operator = (const TUtf16String& wideText) {
            WideText = wideText;
            Text = TString();
            return *this;
        }

        const TString& GetUtf8() const {
            if (!Text && WideText) {
                Text = WideToUTF8(WideText);
            }
            return Text;
        }

        const TUtf16String& GetWide() const {
            if (!WideText && Text) {
                WideText = UTF8ToWide(Text);
            }
            return WideText;
        }

        bool Empty() const {
            return WideText.empty() && Text.empty();
        }
    };

    struct TFormData {
        TText Text;
        bool Exact = false;
    };

    struct TLemmaData {
        NLingBoost::TRevFreq RevFreq; // Rev freq for all lemma occurrences
        TText Text;
        bool Best = false;
        bool Attribute = false;
        ELanguage Language = LANG_UNK;

        TVector<TFormData> Forms;
    };

    struct TWordData {
        NLingBoost::TRevFreq RevFreq; // Rev freq for exact word occurrences
        NLingBoost::TRevFreq RevFreqAllForms; // Rev freq for all word occurrences
        TText Text;
        TCharCategory CaseFlags = CC_EMPTY;
        TLangMask LangMask;
        NLP_TYPE NlpType = NLP_END;
        bool StopWord = false;
        bool AnyWord = false;

        TVector<TLemmaData> Lemmas;
    };

    enum EBlockType {
        Unordered = 0,
        ExactOrdered = 1,
    };

    struct TBlockData {
        size_t Distance = 0;
        EBlockType Type = EBlockType::Unordered;

        TVector<TWordData> Words;
    };

    struct TBinaryBlockData {
        TBlob Data;
        ui64 Hash = 0;
    };

    size_t GetWordsCountForExactOrderedBlock(const TRichRequestNode& node); // return > TSizeLimits::MaxNumWordsInBlock if too many words
    bool IsAnyWordsProximity(const TProximity& prox, size_t& anyWordsCount);

    TWordData& AddBlockWord(TBlockData& data, TUtf16String text, TLangMask langMask,
        TCharCategory caseFlags, NLP_TYPE nlpType, bool stopWord);
    void AddBlockAnyWords(TBlockData& data, size_t anyWordsCount);
    TLemmaData& AddWordLemma(TWordData& data, TUtf16String text, ELanguage language, bool best);
    TFormData& AddLemmaForm(TLemmaData& data, TUtf16String text, bool exact);

    void ParseLemmaForms(const TLemmaForms& lemmaForms, TLemmaData& data);
    void ParseWordNode(const TWordNode& node, TWordData& data);
    void ParseRichNode(const TRichRequestNode& node, TWordData& data);
    void ParseAttributeRichNode(const TRichRequestNode& node, TWordData& data);
    void MakeRichNode(const TWordData& data, TRichNodePtr& node);
    void ParseWordNode(const TWordNode& node, TBlockData& data);
    void ParseRichNode(const TRichRequestNode& node, TBlockData& data);
    void MakeRichNode(const TBlockData& data, TRichNodePtr& node);
} // NDetail
} // NReqBundle
