#pragma once

#include "block_contents.h"
#include "reqbundle_fwd.h"

#include <kernel/idf/idf.h>

#include <util/generic/string.h>

namespace NReqBundle {
    template <typename DataType>
    class TFormAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TFormAccBase() = default;
        TFormAccBase(const TFormAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TFormAccBase(const TConstFormAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TFormAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        const TString& GetText() const {
            return Contents().Text.GetUtf8();
        }
        const TUtf16String& GetWideText() const {
            return Contents().Text.GetWide();
        }
        bool IsExact() const {
            return Contents().Exact;
        }
    };

    template <typename DataType>
    class TLemmaAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TLemmaAccBase() = default;
        TLemmaAccBase(const TLemmaAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TLemmaAccBase(const TConstLemmaAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TLemmaAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        float GetIdf(NLingBoost::EWordFreqType type = NLingBoost::TRevFreq::Default) const {
            return ReverseFreq2Idf(GetRevFreq(type));
        }
        long GetRevFreq(NLingBoost::EWordFreqType type = NLingBoost::TRevFreq::Default) const {
            return Contents().RevFreq.Values[type];
        }
        void SetRevFreq(NLingBoost::EWordFreqType type, long value) const {
            Y_ASSERT(NLingBoost::InvalidRevFreq == value || NLingBoost::IsValidRevFreq(value));
            Contents().RevFreq.Values[type] = value;
        }
        void SetRevFreq(long value) const {
            SetRevFreq(NLingBoost::TRevFreq::Default, value);
        }
        const NLingBoost::TRevFreqsByType& GetRevFreqsByType() const {
            return Contents().RevFreq.Values;
        }

        const TString& GetText() const {
            return Contents().Text.GetUtf8();
        }
        const TUtf16String& GetWideText() const {
            return Contents().Text.GetWide();
        }
        bool IsBest() const {
            return Contents().Best;
        }
        bool IsAttribute() const {
            return Contents().Attribute;
        }
        ELanguage GetLanguage() const {
            return Contents().Language;
        }

        size_t GetNumForms() const {
            return Contents().Forms.size();
        }
        TFormAcc Form(size_t formIndex) const {
            return TFormAcc(Contents().Forms[formIndex]);
        }
        TConstFormAcc GetForm(size_t formIndex) const {
            return TConstFormAcc(Contents().Forms[formIndex]);
        }

        auto Forms() const {
            return NDetail::MakeAccContainer<TFormAcc>(Contents().Forms);
        }
        auto GetForms() const {
            return NDetail::MakeAccContainer<TConstFormAcc>(Contents().Forms);
        }

        void StripAllForms() const {
            Contents().Forms.clear();
        }
    };

    template <typename DataType>
    class TWordAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TWordAccBase() = default;
        TWordAccBase(const TWordAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TWordAccBase(const TConstWordAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TWordAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}
        TWordAccBase& operator=(const TWordAccBase&) = default;

        void MakeRichNode(TRichNodePtr& node) const {
            NDetail::MakeRichNode(Contents(), node);
        }
        void SetRichNode(const TRichRequestNode& node) const {
            NDetail::ParseRichNode(node, Contents());
        }
        void SetWordNode(const TWordNode& node) const {
            NDetail::ParseWordNode(node, Contents());
        }
        float GetIdf(NLingBoost::EWordFreqType type = NLingBoost::TRevFreq::Default) const {
            return ReverseFreq2Idf(GetRevFreq(type));
        }
        long GetRevFreq(NLingBoost::EWordFreqType type = NLingBoost::TRevFreq::Default) const {
            return Contents().RevFreq.Values[type];
        }
        void SetRevFreq(NLingBoost::EWordFreqType type, long value) const {
            Y_ASSERT(NLingBoost::InvalidRevFreq == value || NLingBoost::IsValidRevFreq(value));
            Contents().RevFreq.Values[type] = value;
        }
        void SetRevFreq(long value) const {
            SetRevFreq(NLingBoost::TRevFreq::Default, value);
        }
        const NLingBoost::TRevFreqsByType& GetRevFreqsByType() const {
            return Contents().RevFreq.Values;
        }

        long GetRevFreqAllForms(NLingBoost::EWordFreqType type = NLingBoost::TRevFreq::Default) const {
            return Contents().RevFreqAllForms.Values[type];
        }
        void SetRevFreqAllForms(NLingBoost::EWordFreqType type, long value) const {
            Y_ASSERT(NLingBoost::InvalidRevFreq == value || NLingBoost::IsValidRevFreq(value));
            Contents().RevFreqAllForms.Values[type] = value;
        }
        void SetRevFreqAllForms(long value) const {
            SetRevFreqAllForms(NLingBoost::TRevFreq::Default, value);
        }
        const NLingBoost::TRevFreqsByType& GetRevFreqsAllFormsByType() const {
            return Contents().RevFreqAllForms.Values;
        }

        const TString& GetText() const {
            return Contents().Text.GetUtf8();
        }
        const TUtf16String& GetWideText() const {
            return Contents().Text.GetWide();
        }
        TLangMask GetLangMask() const {
            return Contents().LangMask;
        }
        void SetLangMask(const TLangMask& mask) const {
            Contents().LangMask = mask;
        }
        NLP_TYPE GetNlpType() const {
            return Contents().NlpType;
        }
        TCharCategory GetCaseFlags() const {
            return Contents().CaseFlags;
        }
        bool IsStopWord() const {
            return Contents().StopWord;
        }
        bool IsAnyWord() const {
            return Contents().AnyWord;
        }

        size_t GetNumLemmas() const {
            return Contents().Lemmas.size();
        }
        TLemmaAcc Lemma(size_t lemmaIndex) const {
            return TLemmaAcc(Contents().Lemmas[lemmaIndex]);
        }
        TConstLemmaAcc GetLemma(size_t lemmaIndex) const {
            return TConstLemmaAcc(Contents().Lemmas[lemmaIndex]);
        }

        auto Lemmas() const {
            return NDetail::MakeAccContainer<TLemmaAcc>(Contents().Lemmas);
        }
        auto GetLemmas() const {
            return NDetail::MakeAccContainer<TConstLemmaAcc>(Contents().Lemmas);
        }

        void StripAllForms() const {
            for (auto lemma : Lemmas()) {
                lemma.StripAllForms();
            }
        }
        size_t GetTotalNumForms() const {
            size_t count = 0;
            for (auto lemma : GetLemmas()) {
                count += lemma.GetNumForms();
            }
            return count;
        }
    };

    template <typename DataType>
    class TAttributeAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TAttributeAccBase() = default;
        TAttributeAccBase(const TAttributeAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TAttributeAccBase(const TConstAttributeAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TAttributeAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        const TString& GetName() const {
            return Contents().Name.GetUtf8();
        }

        const TUtf16String& GetWideName() const {
            return Contents().Name.GetWide();
        }

        const TString& GetValue() const {
            return Contents().Value.GetUtf8();
        }

        const TUtf16String& GetWideValue() const {
            return Contents().Value.GetWide();
        }
    };

    template <typename DataType>
    class TBlockAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TBlockAccBase() = default;
        TBlockAccBase(const TBlockAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TBlockAccBase(const TConstBlockAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TBlockAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}
        TBlockAccBase& operator=(const TBlockAccBase&) = default;

        void MakeRichNode(TRichNodePtr& node) const {
            NDetail::MakeRichNode(Contents(), node);
        }
        void SetRichNode(const TRichRequestNode& node) const {
            NDetail::ParseRichNode(node, Contents());
        }
        void AddRichNode(const TRichRequestNode& node) const {
            Contents().Words.emplace_back();
            NDetail::ParseRichNode(node, Contents().Words.back());
        }
        void SetWordNode(const TWordNode& node) const {
            NDetail::ParseWordNode(node, Contents());
        }
        void AddWordNode(const TWordNode& node) const {
            Contents().Words.emplace_back();
            NDetail::ParseWordNode(node, Contents().Words.back());
        }

        void AppendWord(const TConstWordAcc& word) {
            Contents().Words.push_back(NDetail::BackdoorAccess(word));
        }

        void AppendBlockWords(const TConstBlockAcc& block) {
            for (size_t i = 0; i < block.GetNumWords(); ++i) {
                AppendWord(block.GetWord(i));
            }
        }

        size_t GetDistance() const {
            return Contents().Distance;
        }

        NDetail::EBlockType GetType() const {
            return Contents().Type;
        }

        size_t GetNumWords() const {
            return Contents().Words.size();
        }
        bool IsWordBlock() const {
            return GetNumWords() == 1;
        }
        TWordAcc Word(size_t wordIndex = 0) const {
            return TWordAcc(Contents().Words[wordIndex]);
        }
        TConstWordAcc GetWord(size_t wordIndex = 0) const {
            return TConstWordAcc(Contents().Words[wordIndex]);
        }

        auto Words() const {
            return NDetail::MakeAccContainer<TWordAcc>(Contents().Words);
        }
        auto GetWords() const {
            return NDetail::MakeAccContainer<TConstWordAcc>(Contents().Words);
        }

        void StripAllForms() {
            for (auto word : Words()) {
                word.StripAllForms();
            }
        }
    };

    template <typename DataType>
    class TBinaryBlockAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TBinaryBlockAccBase() = default;
        TBinaryBlockAccBase(const TBinaryBlockAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TBinaryBlockAccBase(const TConstBinaryBlockAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TBinaryBlockAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        TBlob GetData() const {
            return Contents().Data;
        }
        ui64 GetHash() const {
            return Contents().Hash;
        }
        bool Size() const {
            return Contents().Data.Size();
        }
    };

    int Compare(TConstWordAcc x, TConstWordAcc y);
    int Compare(TConstBlockAcc x, TConstBlockAcc y);

    inline bool operator < (TConstWordAcc x, TConstWordAcc y) {
        return Compare(x, y) < 0;
    }
    inline bool operator < (TConstBlockAcc x, TConstBlockAcc y) {
        return Compare(x, y) < 0;
    }

    namespace NDetail {
        EValidType IsValidLemma(TConstLemmaAcc lemma);
        EValidType IsValidWord(TConstWordAcc word);
        EValidType IsValidBlock(TConstBlockAcc block);
    }
} // NReqBundle
