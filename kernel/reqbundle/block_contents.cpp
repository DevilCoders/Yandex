#include <kernel/search_types/search_types.h>
#include "block_contents.h"
#include "size_limits.h"

#include <library/cpp/langmask/serialization/langmask.h>
#include <ysite/yandex/common/prepattr.h>

#include <kernel/qtree/richrequest/protos/rich_tree.pb.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/wordnode.h>

namespace NReqBundle {
namespace NDetail {
    size_t GetWordsCountForExactOrderedBlock(const TRichRequestNode& node) {
        size_t wordsCount = node.Children.size();
        for (size_t wordIndex = 0; wordIndex + 1 < node.Children.size() && wordsCount <= TSizeLimits::MaxNumWordsInBlock; ++wordIndex) {
            size_t anyWordsCount = 0;
            if (IsAnyWordsProximity(node.Children.ProxAfter(wordIndex), anyWordsCount)) {
                wordsCount += anyWordsCount;
            }
        }
        return wordsCount;
    }

    bool IsAnyWordsProximity(const TProximity& prox, size_t& anyWordsCount) {
        if (prox.Level == BREAK_LEVEL && prox.Beg == prox.End && prox.Beg >= 2) {
            anyWordsCount = prox.Beg - 1;
            return true;
        } else {
            return false;
        }
    }

    TWordData& AddBlockWord(TBlockData& data, TUtf16String text, TLangMask langMask,
        TCharCategory caseFlags, NLP_TYPE nlpType, bool stopWord)
    {
        data.Words.emplace_back();
        auto& word = data.Words.back();
        word.Text = text;
        word.LangMask = langMask;
        word.CaseFlags = caseFlags;
        word.NlpType = nlpType;
        word.StopWord = stopWord;
        return word;
    }

    void AddBlockAnyWords(TBlockData& data, size_t anyWordsCount) {
        for (size_t i = 0; i < anyWordsCount; ++i) {
            data.Words.emplace_back();
            data.Words.back().AnyWord = true;
        }
    }

    TLemmaData& AddWordLemma(TWordData& data, TUtf16String text, ELanguage language, bool best)
    {
        data.Lemmas.emplace_back();
        auto& lemma = data.Lemmas.back();
        lemma.Text = text;
        lemma.Language = language;
        lemma.Best = best;
        return lemma;
    }

    TFormData& AddLemmaForm(TLemmaData& data, TUtf16String text, bool exact)
    {
        data.Forms.emplace_back();
        auto& form = data.Forms.back();
        form.Text = text;
        form.Exact = exact;
        return form;
    }

    void ParseLemmaForms(const TLemmaForms& lemmaForms, TLemmaData& data)
    {
        data.Text = lemmaForms.GetLemma();
        data.Language = lemmaForms.GetLanguage();
        data.Best = lemmaForms.IsBest();

        if (lemmaForms.FormsGenerated()) {
            const TLemmaForms::TFormMap& formMap = lemmaForms.GetForms();
            data.Forms.resize(formMap.size());

            size_t formIndex = 0;
            for (auto iter = formMap.begin(); iter != formMap.end(); ++iter, ++formIndex) {
                TFormData& form = data.Forms[formIndex];

                form.Text = iter->first;
                form.Exact = iter->second.IsExact;
            }
        }
    }

    void ParseWordNode(const TWordNode& node, TWordData& data)
    {
        data.Text = node.GetNormalizedForm();
        data.LangMask = node.GetLangMask();
        data.CaseFlags = node.GetCaseFlags();
        data.StopWord = node.IsStopWord();

        const size_t numLemmas = node.GetLemmas().size();
        data.Lemmas.resize(numLemmas);

        for (size_t lemmaIndex = 0; lemmaIndex != numLemmas; ++lemmaIndex) {
            const TLemmaForms& lemmaForms = node.GetLemmas()[lemmaIndex];
            TLemmaData& lemma = data.Lemmas[lemmaIndex];
            ParseLemmaForms(lemmaForms, lemma);
        }

        if (node.IsLemmerWord()) {
            data.NlpType = NLP_WORD;
        } else if (node.IsInteger()) {
            data.NlpType = NLP_INTEGER;
        } else if (data.Text.GetWide()) {
            data.NlpType = GuessTypeByWord(data.Text.GetWide().data(), data.Text.GetWide().size());
            Y_ASSERT(data.NlpType != NLP_END);
        } else {
            data.NlpType = NLP_END;
        }
    }

    void ParseRichNode(const TRichRequestNode& node, TWordData& data)
    {
        Y_ASSERT(node.Children.size() == 0);

        if (node.WordInfo) {
            ParseWordNode(*node.WordInfo, data);
        }

        if (!data.Text.GetWide()) {
            data.Text = node.GetText();
        }

        const TTokenStructure& subTokens = node.GetSubTokens();
        if (data.NlpType == NLP_END && subTokens.size() > 0) {
            switch (subTokens[0].Type) {
                case TOKEN_WORD: {
                    data.NlpType = NLP_WORD;
                    break;
                }
                case TOKEN_NUMBER: {
                    data.NlpType = NLP_INTEGER;
                    break;
                }
                default: {
                    if (data.Text.GetWide()) {
                        data.NlpType = GuessTypeByWord(data.Text.GetWide().data(), data.Text.GetWide().size());
                        Y_ASSERT(data.NlpType != NLP_END);
                    } else {
                        Y_ASSERT(data.NlpType == NLP_END);
                    }
                    break;
                }
            }
        }
    }

    void MakeRichNode(const TWordData& data, TRichNodePtr& node)
    {
        NRichTreeProtocol::TRichRequestNode message;

        NRichTreeProtocol::TRequestNodeBase& nodeBase = *message.MutableBase();
        NRichTreeProtocol::TWordNode& wordInfo = *message.MutableWordInfo();
        NRichTreeProtocol::TOpInfo& opInfo = *nodeBase.MutableOpInfo();

        nodeBase.SetText(data.Text.GetUtf8());
        nodeBase.SetReverseFreq(-1);

        NProto::TCharSpan& subTok = *nodeBase.AddSubTokens();
        subTok.SetLen(data.Text.GetWide().size());
        subTok.SetType(data.NlpType == NLP_WORD ? TOKEN_WORD : TOKEN_NUMBER);

        opInfo.SetOp(8); // "oWord", see kernel/qtree/richrequest/serializer/io.cpp
        opInfo.SetCmpOper(cmpEQ);

        ::Serialize(*wordInfo.MutableLanguages(), data.LangMask, false);

        if (data.Lemmas.size() > 0) {
            wordInfo.SetNlpType(data.NlpType);
            wordInfo.SetStopWord(data.StopWord);
            wordInfo.SetFormType(fGeneral);
            wordInfo.SetForm(data.Text.GetUtf8());

            wordInfo.SetCaseFlags(data.CaseFlags);
            wordInfo.SetTokenCount(1);
            wordInfo.SetRevFr(-1);
        }

        if (data.NlpType == NLP_INTEGER) {
            wchar16 attrBuf[MAXKEY_BUF];
            PrepareInteger(data.Text.GetWide().data(), attrBuf);
            wordInfo.SetAttr(WideToUTF8(attrBuf));
        } else {
            for (size_t lemmaIndex = 0; lemmaIndex != data.Lemmas.size(); ++lemmaIndex) {
                NRichTreeProtocol::TLemma& lemmaProto = *wordInfo.AddLemmas();
                const TLemmaData& lemma = data.Lemmas[lemmaIndex];
                lemmaProto.SetLemma(lemma.Text.GetUtf8());
                lemmaProto.SetBinLanguage(lemma.Language);
                lemmaProto.SetBest(lemma.Best);
                lemmaProto.SetStickiness(STICK_NONE);

                for (size_t formIndex = 0; formIndex != lemma.Forms.size(); ++formIndex) {
                    NRichTreeProtocol::TForm& formProto = *lemmaProto.AddForms();
                    const TFormData& form = lemma.Forms[formIndex];
                    formProto.SetForm(form.Text.GetUtf8());

                    if (form.Exact) {
                        formProto.SetMode(0x10); // ModeExactWord, see kernel/qtree/richrequest/wordnode.cpp
                    }
                }
            }
        }

        message.SetHiliteType(HILITE_SELF);

        node = Deserialize(message);

        Y_ASSERT(node);
    }

    void ParseWordNode(const TWordNode& node, TBlockData& data)
    {
        data.Words.resize(1);
        ParseWordNode(node, data.Words[0]);
    }

    void ParseAttributeRichNode(const TRichRequestNode& node, TWordData& data)
    {
        static const TUtf16String Separator = u"=";

        Y_ASSERT(IsAttribute(node));

        data.Text = node.GetTextName() + Separator + node.GetText();

        data.Lemmas.resize(1);
        data.Lemmas[0].Attribute = true;
        data.Lemmas[0].Text = data.Text;
    }

    void ParseRichNode(const TRichRequestNode& node, TBlockData& data)
    {
        if (IsAttribute(node)) {
            data.Words.resize(1);
            ParseAttributeRichNode(node, data.Words[0]);
            return;
        }
        if (node.IsQuoted()) {
            data.Type = EBlockType::ExactOrdered;
        }
        if (node.Children.size() == 0) {
            data.Words.resize(1);
            ParseRichNode(node, data.Words[0]);
        } else {
            size_t childsCount = Min<size_t>(node.Children.size(), TSizeLimits::MaxNumWordsInBlock);
            if (data.Type == EBlockType::ExactOrdered) {
                size_t wordsCount = GetWordsCountForExactOrderedBlock(node);
                if (wordsCount > TSizeLimits::MaxNumWordsInBlock) {
                    data.Type = EBlockType::Unordered;
                    wordsCount = childsCount;
                }
                data.Words.reserve(wordsCount);
            } else {
                data.Words.reserve(childsCount);
            }
            size_t distance = 0;
            for (size_t wordIndex = 0; wordIndex < childsCount; ++wordIndex) {
                data.Words.emplace_back();
                ParseRichNode(*node.Children[wordIndex], data.Words.back());
                if (wordIndex + 1 < childsCount) {
                    TProximity prox = node.Children.ProxAfter(wordIndex);
                    if (prox.Level == BREAK_LEVEL) {
                        distance = Max<size_t>(distance, abs(prox.Beg));
                        distance = Max<size_t>(distance, abs(prox.End));
                        size_t anyWordsCount = 0;
                        if (data.Type == EBlockType::ExactOrdered && IsAnyWordsProximity(prox, anyWordsCount)) {
                            AddBlockAnyWords(data, anyWordsCount);
                        }
                    }
                }
            }
            if (data.Type == EBlockType::Unordered) {
                data.Distance = distance;
            }
        }
    }

    void MakeRichNode(const TBlockData& data, TRichNodePtr& node)
    {
        if (data.Type == EBlockType::ExactOrdered) {
            node->SetQuoted();
        }

        if (data.Words.size() == 1) {
            MakeRichNode(data.Words[0], node);
            return;
        }

        node = CreateEmptyRichNode();

        node->OpInfo = SentencePhraseOpInfo;
        node->Parens = true;
        node->SetPhraseType(PHRASE_USEROP);

        size_t wordIndex = 0;
        while (wordIndex < data.Words.size()) {
            Y_ASSERT(!data.Words[wordIndex].AnyWord);
            TRichNodePtr childNode;
            MakeRichNode(data.Words[wordIndex], childNode);
            node->Children.Append(childNode);
            if (wordIndex + 1 >= data.Words.size()) {
                break;
            }
            size_t lastChildIndex = node->Children.size() - 1;
            if (data.Type == EBlockType::ExactOrdered) {
                size_t firstNonAnyWordPtr = wordIndex + 1;
                while (firstNonAnyWordPtr < data.Words.size() && data.Words[firstNonAnyWordPtr].AnyWord) {
                    ++firstNonAnyWordPtr;
                }
                Y_ASSERT(firstNonAnyWordPtr < data.Words.size());
                size_t anyWordsCount = firstNonAnyWordPtr - wordIndex - 1;
                node->Children.ProxAfter(lastChildIndex) = TProximity(anyWordsCount + 1, anyWordsCount + 1);
                size_t checkAnyWordsCount = 0;
                Y_ASSERT(IsAnyWordsProximity(node->Children.ProxAfter(lastChildIndex), checkAnyWordsCount));
                Y_ASSERT(anyWordsCount == checkAnyWordsCount);
                wordIndex = firstNonAnyWordPtr;
            } else {
                if (data.Distance) {
                    node->Children.ProxAfter(lastChildIndex) = TProximity(-static_cast<int>(data.Distance), +static_cast<int>(data.Distance));
                } else {
                    node->Children.ProxAfter(lastChildIndex) = TProximity(BREAK_LEVEL);
                }
                ++wordIndex;
            }
        }

        Y_ASSERT(node);
    }
} // NDetail
} // NReqBundle
