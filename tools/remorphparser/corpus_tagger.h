#pragma once

#include <dict/corpus/corpus.h>

#include <kernel/remorph/tokenizer/callback.h>
#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/facts/facttype.h>

#include <util/generic/typetraits.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/defaults.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

namespace NRemorphParser {

template <bool MultiThreaded>
class TCorpusTagger {
private:
    typedef std::conditional_t<MultiThreaded, TMutex, TFakeMutex> TPseudoMutex;
    typedef THashSet<TString> TTags;
    typedef TVector<NFact::TFactPtr> TFacts;
    typedef TVector<NFact::TFieldTypePtr> TFieldTypes;
    typedef TVector<NFact::TFieldValuePtr> TFieldValues;
    typedef TVector<NFact::TCompoundFieldValuePtr> TCompoundFieldValues;

private:
    TPseudoMutex Mutex;
    NCorpus::TJsonCorpus& Corpus;
    TTags Tags;
    NCorpus::TCorpus::TText* Text;

public:
    explicit TCorpusTagger(NCorpus::TJsonCorpus& corpus)
        : Mutex()
        , Corpus(corpus)
        , Tags()
        , Text(nullptr)
    {
    }

    template <class TSymbolPtr>
    inline void operator()(const NToken::TSentenceInfo& sentenceInfo, const TVector<TSymbolPtr>& symbols,
                           const TFacts& facts) {
        TGuard<TPseudoMutex> guard(Mutex);
        TagFacts(sentenceInfo, symbols, facts);
    }

    inline void SetHeader(const TString* title) {
        if (title) {
            Corpus.Settitle(*title);
        }
    }

    inline void NewText(ui32 textId) {
        Text = Corpus.Addtexts();
        Text->Setid(textId);
        Text->Settagged(true);
    }

private:
    inline void Tag(const TString& fact, size_t begin, size_t end) {
        if (!Text) {
            Text = Corpus.Addtexts();
            Text->Setid(0);
            Text->Settagged(true);
        }
        NCorpus::TCorpus::TText::TTag* tag = Text->Addtags();
        tag->Setfact(fact);
        tag->Setbegin(begin);
        tag->Setend(end);

        std::pair<TTags::const_iterator, bool> tagInsertResult = Tags.insert(fact);
        if (tagInsertResult.second) {
            Corpus.Addtags(fact);
        }
    }

    template <class TSymbolPtr>
    inline void TagRange(const NToken::TSentenceInfo& sentenceInfo, const TVector<TSymbolPtr>& symbols,
                         const TString& name, const NFact::TRangeInfo& range) {
        Y_ASSERT(range.GetSrcPos().first < symbols.size());
        Y_ASSERT(range.GetSrcPos().second <= symbols.size());
        const size_t begin = sentenceInfo.Pos.first + symbols[range.GetSrcPos().first]->GetSentencePos().first;
        const size_t end = sentenceInfo.Pos.first + symbols[range.GetSrcPos().second - 1]->GetSentencePos().second;
        Tag(name, begin, end);
    }

    template <class TSymbolPtr>
    inline void TagFacts(const NToken::TSentenceInfo& sentenceInfo, const TVector<TSymbolPtr>& symbols,
                         const TFacts& facts) {
        for (TFacts::const_iterator iFact = facts.begin(); iFact != facts.end(); ++iFact) {
            const NFact::TFact& fact = **iFact;
            TString name = fact.GetType().GetTypeName();
            TagRange(sentenceInfo, symbols, name, fact);
            TagFields(sentenceInfo, symbols, name + ':', fact.GetType(), fact);
        }
    }

    template <class TSymbolPtr>
    void TagFields(const NToken::TSentenceInfo& sentenceInfo, const TVector<TSymbolPtr>& symbols,
                   const TString& prefix, const NFact::TFieldTypeContainer& fieldType,
                   const NFact::TFieldValueContainer& fieldValue) {
        const TFieldTypes& types = fieldType.GetFields();
        for (TFieldTypes::const_iterator iType = types.begin(); iType != types.end(); ++iType) {
            const NFact::TFieldType& type = **iType;
            if (type.IsCompound()) {
                const TCompoundFieldValues& fields = fieldValue.GetCompoundValues(type.GetName());
                for (TCompoundFieldValues::const_iterator iField = fields.begin(); iField != fields.end(); ++iField) {
                    const NFact::TCompoundFieldValue& field = **iField;
                    TagFields(sentenceInfo, symbols, prefix + type.GetName() + '.', type, field);
                }
            } else {
                const TFieldValues& fields = fieldValue.GetValues(type.GetName());
                for (TFieldValues::const_iterator iField = fields.begin(); iField != fields.end(); ++iField) {
                    TagRange(sentenceInfo, symbols, prefix + type.GetName(), **iField);
                }
            }
        }
    }
};

} // NRemorphParser
