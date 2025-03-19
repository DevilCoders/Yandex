#include "richnodeiter.h"


namespace NGzt {


bool IsNodeText(const TRichRequestNode* node, const TUtf16String& text) {
    TUtf16String nodeText = GetLowerCaseRichRequestNodeText(node);
    return nodeText == text || TMultitokenLemmaIterator::IsArtificialText(node, text);
}




void TMultitokenLemmaIterator::Init() {
    const TWordNode* wordInfo = Node->WordInfo.Get();
    if (wordInfo != nullptr && wordInfo->IsLemmerWord())
        It = TDefaultWordInstanceLemmaIter(wordInfo->LemsBegin(), wordInfo->LemsEnd());
    else
        BuildArtificial(Node, ArtificialLemmas);
}

bool TMultitokenLemmaIterator::Ok() const {
    if (ArtificialLemmas.empty())
        return It.Ok();
    else
        return CurArtificialLemma < ArtificialLemmas.ysize();
}

void TMultitokenLemmaIterator::operator++() {
    if (ArtificialLemmas.empty())
        ++It;
    else
        ++CurArtificialLemma;
}

TUtf16String TMultitokenLemmaIterator::GetLemmaText() const {
    if (ArtificialLemmas.empty())
        return It->GetLemma();
    else
        return ArtificialLemmas[CurArtificialLemma].Text;
}

TStemFlexGramIterator TMultitokenLemmaIterator::GetGrammems() const {
    if (ArtificialLemmas.empty())
        return TStemFlexGramIterator(It->GetStemGrammar(), It->GetFlexGrammars());
    else
        return TStemFlexGramIterator(ArtificialLemmas[CurArtificialLemma].StemGram,
                                     ArtificialLemmas[CurArtificialLemma].FlexGram);
}

ELanguage TMultitokenLemmaIterator::GetLanguage() const {
    if (ArtificialLemmas.empty())
        return It->GetLanguage();
    else
        return LANG_UNK;
}

TUtf16String TMultitokenLemmaIterator::ArtificialPrefix(const TRichRequestNode* node) {
    TUtf16String prefix;
    for (int i = 0; i < (int)node->Children.size() - 1; ++i) {
        const TRichRequestNode& childNode = *node->Children[i];
        if (!childNode.WordInfo)
            continue;
        prefix += childNode.WordInfo->GetNormalizedForm();
        prefix += childNode.GetPunctAfter();
    }
    return prefix;
}

bool TMultitokenLemmaIterator::Check(const TRichRequestNode* node) {
    if (node->GetPhraseType() != PHRASE_MULTIWORD || node->Children.empty())
        return false;

    const TRichRequestNode& lastChild = *node->Children.back();
    return lastChild.WordInfo.Get() != nullptr && lastChild.WordInfo->IsLemmerWord();
}

void TMultitokenLemmaIterator::BuildArtificial(const TRichRequestNode* node,
                                               TVector<TArtificialLemma>& res) {
    if (!Check(node))
        return;

    TUtf16String prefix = ArtificialPrefix(node);
    const TRichRequestNode& lastChild = *node->Children.back();

    TWordInstance::TCLemIterator it = lastChild.WordInfo->LemsBegin();
    for(; it != lastChild.WordInfo->LemsEnd(); ++it) {
        const TLemmaForms& lemma = *it;
        TArtificialLemma artLemma(prefix + lemma.GetLemma(), lemma.GetFlexGrammars(), lemma.GetStemGrammar());
        res.push_back(artLemma);
    }
}

bool TMultitokenLemmaIterator::IsArtificialText(const TRichRequestNode* node, const TUtf16String& text) {
    if (!Check(node))
        return false;

    TUtf16String prefix = ArtificialPrefix(node);
//    prefix.to_lower();
    if (!text.StartsWith(prefix))
        return false;

    TWtringBuf suffix = TWtringBuf(text).SubStr(prefix.size());

    const TRichRequestNode& lastChild = *node->Children.back();
    TWordInstance::TCLemIterator it = lastChild.WordInfo->LemsBegin();
    TUtf16String artText;
    for(; it != lastChild.WordInfo->LemsEnd(); ++it) {
        artText = it->GetLemma();
//        artText.to_lower();
        if (TWtringBuf(artText) == suffix)
            return true;
    }
    return false;
}

void CollectUniqForms(const TRichRequestNode& node, TVector<TUtf16String>& forms, TSet<TUtf16String>& tmp) {
    tmp.clear();
    forms.clear();
    if (node.WordInfo.Get() != nullptr)
        for (TWordInstance::TCLemIterator it = node.WordInfo->LemsBegin(); it != node.WordInfo->LemsEnd(); ++it)
            if (it->GetNormalizedForm() != node.WordInfo->GetNormalizedForm())    // skip main wordnode forma, as it will be used for GetWordString()
                tmp.insert(it->GetNormalizedForm());
    forms.insert(forms.end(), tmp.begin(), tmp.end());
}

void TExtraFormsRichNodes::CollectForms() {
    ExtraForms.resize(Nodes.size());
    TSet<TUtf16String> tmp;
    for (size_t i = 0; i < Nodes.size(); ++i) {
        CollectUniqForms(*Nodes[i], ExtraForms[i], tmp);
/*        Cerr << "Extra forms:";
        for (size_t j = 0; j < ExtraForms[i].size(); ++j)
            Cerr << " " << ExtraForms[i][j];
        Cerr << Endl;
*/
    }
}

}   // namespace NGzt
