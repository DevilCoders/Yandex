#include "richtreebuilderaux.h"

#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/search_daemon_iface/reqtypes.h>

#include <library/cpp/tokenclassifiers/token_markup.h>
#include <library/cpp/tokenclassifiers/token_types.h>
#include <library/cpp/tokenclassifiers/classifiers/email_classifier.h>
#include <library/cpp/tokenclassifiers/classifiers/url_classifier.h>
#include <library/cpp/tokenclassifiers/token_classifiers_singleton.h>
#include <library/cpp/unicode/punycode/punycode.h>

#include <util/generic/ptr.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <library/cpp/string_utils/quote/quote.h>


namespace NRichTreeBuilder {

static bool HasMixedChild(const TRichNodePtr& parent) {
    for (size_t i = 0; i < parent->Children.size(); ++i) {
        if (IsMixed(parent->Children[i]))
            return true;
    }
    return false;
}

static TRichNodePtr GetWholeTextNode(const TRichNodePtr& node) {
    TRichNodePtr ndWhl (TTreeCreator::NewNode(*node, node->Necessity, node->GetFormType()));
    if (!!node->GetText())
        ndWhl->SetLeafWord(node->GetText(), node->GetTokSpan());
    else
        ndWhl->SetLeafWord(node->WordInfo->GetNormalizedForm(), node->GetTokSpan());
    ndWhl->OpInfo = node->OpInfo;
    ndWhl->OpInfo.Op = oUnused;
    return ndWhl;
}

static void ReplaceWithChildren(TRichNodePtr node, TRichNodePtr parent) {
    TRichNodePtr ndWhl = GetWholeTextNode(node);

    size_t pos = parent->GetChildPos(*node);
    size_t numChilds = node->Children.size();

    Y_ASSERT(numChilds > 0);

    if (node.Get() != parent.Get() && IsAndOp(*parent)) {
        Y_ASSERT(pos < parent->Children.size());
        parent->Children.Replace(pos, pos + 1, node->Children);
    } else {
        pos = 0;
        parent = node;
        parent->SetPhrase(PHRASE_PHRASE);
    }
    parent->AddMarkup(pos, pos + numChilds - 1, new TSynonym(ndWhl, TE_MARK, 0, EQUAL_BY_STRING));
}

static void ReplaceWithMixedToken(TRichNodePtr node, TRichNodePtr parent) {
    TCharSpan span = node->GetTokSpan();
    span.Type = TOKEN_MIXED;
    TUtf16String text = node->GetText();
    node->SetLeafWord(text, span);

    TRichNodePtr ndAnd = CreateEmptyRichNode();
    ndAnd->SetPhrase(PHRASE_PHRASE);

    ndAnd->Children.Swap(node->Children);

    size_t pos = parent->GetChildPos(*node);
    parent->AddMarkup(pos, pos, new TTechnicalSynonym(ndAnd, TE_MARK2, 0, EQUAL_BY_STRING));
}


static void ReplaceMarkSubtree(TRichNodePtr& node, TRichNodePtr& parent) {
    if (HasMixedChild(node))
        ReplaceWithMixedToken(node, parent);
    else
        ReplaceWithChildren(node, parent);
}

static void FlatMarks(TRichNodePtr& node, TRichNodePtr&  parent) {
    if (IsNonLemmerMultitoken(node))
        ReplaceMarkSubtree(node, parent);

    for (size_t i = 0; i < node->Children.size();) {
        if (IsNonLemmerMultitoken(node->Children[i]))
            ReplaceMarkSubtree(node->Children.MutableNode(i), node);
        else
            ++i;
    }
    for (size_t i = 0; i < node->Children.size(); ++i) {
        FlatMarks(node->Children.MutableNode(i), node);
    }
}

static const wchar16 STRONG_DISTANCE[] = {' ', '&', '/', '(', '1', ' ', '1', ')', ' ', 0};

static void ClearMiscOps(TRichNodePtr& node) {
    node->MiscOps.clear();
    for (size_t i = 0; i < node->Children.size(); ++i) {
        ClearMiscOps(node->Children.MutableNode(i));
    }
}

static void AddSynonym(TRichRequestNode* parent,
                       TRichRequestNode& node,
                       const TCreateTreeOptions& options,
                       const TUtf16String& synonymText,
                       bool clearMiscOps=false)
{
    TRichNodePtr synonym = CreateRichNode(synonymText, options);
    if (clearMiscOps) {
        ClearMiscOps(synonym);
    }

    if (parent == nullptr) {
        node.AddMarkup(0, 0, new TSynonym(synonym, TE_TOKEN_PART, 0.0, EQUAL_BY_STRING));
    } else {
        parent->AddMarkup(node, new TSynonym(synonym, TE_TOKEN_PART, 0.0, EQUAL_BY_STRING));
    }
}

static void CopyAttributesForEmailSplit(const TRichNodePtr& newNode, const TRichRequestNode& node) {
    newNode->MiscOps = node.MiscOps;
    NTokenClassification::TTokenTypes newTokenTypes = node.GetTokenTypes();
    newTokenTypes |= NTokenClassification::ETT_SPLIT_EMAIL;
    newTokenTypes &= ~NTokenClassification::ETT_EMAIL;
    newNode->SetTokenTypes(newTokenTypes);
    newNode->SetHiliteType(node.GetHiliteType());
    newNode->SetQuoted(node.IsQuoted());
}

static void SplitEmail(TRichRequestNode& node,
                       const NTokenClassification::TMarkupGroup::TWStringLayer& loginWords,
                       const NTokenClassification::TMarkupGroup::TWStringLayer& domainWords,
                       const TCreateTreeOptions& options)
{
    if (!node.Children.empty() || !loginWords || !domainWords)
        return;
    TUtf16String space = TUtf16String((wchar16) ' ');
    TRichNodePtr newNode = CreateRichNode(TUtf16String(), options);
    newNode->Children.Append(CreateRichNode(loginWords[0] + space, options), TProximity(1, 1));
    newNode->Children.ProxBefore(0).DistanceType = DT_USEROP;
    for (size_t i = 1; i < loginWords.size(); ++i) {
        newNode->Children.Append(CreateRichNode(space + loginWords[i] + space, options), TProximity(1, 1));
        newNode->Children.ProxBefore(i).DistanceType = DT_USEROP;
    }
    for (size_t i = 0; i < domainWords.size() - 1; ++i) {
        newNode->Children.Append(CreateRichNode(space + domainWords[i] + space, options), TProximity(1, 1));
        newNode->Children.ProxBefore(loginWords.size() + i).DistanceType = DT_USEROP;
    }
    newNode->Children.Append(CreateRichNode(space + domainWords.back(), options), TProximity(1, 1));
    newNode->Children.ProxBefore(loginWords.size() + domainWords.size() - 1).DistanceType = DT_USEROP;
    newNode->Children.ProxBefore(loginWords.size()).SetDistance(-64, 64, BREAK_LEVEL, DT_USEROP);
    newNode->Children.ProxBefore(loginWords.size()).Center = 0;
    newNode->SetPhrase(PHRASE_USEROP);
    newNode->OpInfo.Lo = 1;
    newNode->OpInfo.Hi = 1;
    newNode->OpInfo.Level = 0;
    CopyAttributesForEmailSplit(newNode, node);
    node.Swap(*newNode);
}

static void AddEmailSynonyms(const TClassifiedTokenIterator& iter,
                             const TCreateTreeOptions& options,
                             const NTokenClassification::TMultitokenMarkupGroups& markupGroups)
{
    using NTokenClassification::TMarkupGroup;
    using namespace NTokenClassification::NEmailClassification;

    TRichRequestNode* parent = &iter.GetParent();
    TRichRequestNode& node  = *iter;

    const TMarkupGroup& domainMarkupGroup = markupGroups.GetMarkupGroup(EEMG_DOMAIN);
    const TMarkupGroup& loginMarkupGroup  = markupGroups.GetMarkupGroup(EEMG_LOGIN);

    TMarkupGroup::TWStringLayer loginWords;
    loginMarkupGroup.GetLayerAsWStringLayer(EELML_ELEMENTARY_TOKENS, loginWords);

    TMarkupGroup::TWStringLayer domainWords;
    domainMarkupGroup.GetLayerAsWStringLayer(EEDML_ELEMENTARY_TOKENS, domainWords);

    if (options.Reqflags & RPF_SPLIT_EMAILS) {
        SplitEmail(node, loginWords, domainWords, options);
    }
    else {
        TUtf16String originalEmail =
            JoinStrings(loginWords, STRONG_DISTANCE) +
            STRONG_DISTANCE + JoinStrings(domainWords, STRONG_DISTANCE);

        AddSynonym(parent, node, options, originalEmail);
    }

}

static void CleanupSynonym(TUtf16String& w) {
    for (size_t i = 0; i < w.size(); ++i) {
        if (w[i] == '|' || w[i] == '^' || w[i] == '~')
            w[i] = ' ';
    }
}

static void AddUrlSynonyms(const TClassifiedTokenIterator& iter,
                           const TCreateTreeOptions& options,
                           const NTokenClassification::TMultitokenMarkupGroups& markupGroups)
{
    using NTokenClassification::TMarkupGroup;
    using namespace NTokenClassification::NUrlClassification;

    TRichRequestNode* parent = &iter.GetParent();
    TRichRequestNode& node  = *iter;

    const TMarkupGroup& hostMarkupGroup = markupGroups.GetMarkupGroup(EUMG_HOST);
    const TMarkupGroup& pathMarkupGroup = markupGroups.GetMarkupGroup(EUMG_PATH);

    TMarkupGroup::TWStringLayer hostWords;
    hostMarkupGroup.GetLayerAsWStringLayer(EUHML_ELEMENTARY_TOKENS, hostWords);
    TMarkupGroup::TWStringLayer pathWords;
    pathMarkupGroup.GetLayerAsWStringLayer(EUPML_ELEMENTARY_TOKENS, pathWords);

    TUtf16String joinedHostTokens = JoinStrings(hostWords, STRONG_DISTANCE);
    TString unescapedPath = WideToUTF8(JoinStrings(pathWords, STRONG_DISTANCE));
    UrlUnescape(unescapedPath);

    TUtf16String originalUrl = joinedHostTokens;
    if (!unescapedPath.empty() && IsUtf(unescapedPath)) {
        originalUrl += STRONG_DISTANCE + UTF8ToWide(unescapedPath);
    }

    CleanupSynonym(originalUrl); // BEGEMOT-173

    AddSynonym(parent, node, options, originalUrl, true);
}

void SplitEmailsAndUrls(TRichNodePtr& root, const TCreateTreeOptions& options) {
    using namespace NTokenClassification;

    for (TClassifiedTokenIterator iter(root); !iter.IsDone(); ++iter) {
        const TUtf16String& text = iter->GetText();
        if (iter->GetTokenTypes() & ETT_URL_AND_EMAIL) {
            TMultitokenMarkupGroups markupGroups;
            TTokenClassifiersSingleton::GetMarkup(text.begin(), text.length(),
            (iter->GetTokenTypes() & ETT_EMAIL) ? ETT_EMAIL : ETT_URL, markupGroups);

            if (iter->GetTokenTypes() & ETT_EMAIL) {
                AddEmailSynonyms(iter, options, markupGroups);
            } else {
                AddUrlSynonyms(iter, options, markupGroups);
            }
        } else if (iter->GetTokenTypes() & ETT_PUNYCODE) {
            TRichRequestNode* parent = &iter.GetParent();
            TRichRequestNode& node  = *iter;
            const TUtf16String& punycode = node.GetText();
            AddSynonym(parent, node, options, punycode);

            TUtf16String original;
            try {
                original = PunycodeToHostName(WideToUTF8(punycode));
            } catch (TPunycodeError&) {
            }

            if (original) {
                AddSynonym(parent, node, options, original);
            }
        }
    }
}

static void PostProcessTreeInt(TRichNodePtr& node, TRichNodePtr& parent, const TCreateTreeOptions& options) {
    if (node.Get() == parent.Get()) {
            TCreateTreeOptions newOpt(options);
            newOpt.Reqflags &= ~RPF_USE_TOKEN_CLASSIFIER;
            SplitEmailsAndUrls(node, newOpt);
    }

    FlatMarks(node, parent);

    for (size_t i = 0; i < node->Children.size(); ++i)
        PostProcessTreeInt(node->Children.MutableNode(i), node, options);

    for (size_t i = 0; i < node->MiscOps.size(); ++i)
        PostProcessTree(node->MiscOps[i], options);

    for (NSearchQuery::TForwardMarkupIterator<TSynonym, false> i(node->MutableMarkup()); !i.AtEnd(); ++i)
        PostProcessTree(i.GetData().SubTree, options);

    for (NSearchQuery::TForwardMarkupIterator<TTechnicalSynonym, false> i(node->MutableMarkup()); !i.AtEnd(); ++i)
        PostProcessTree(i.GetData().SubTree, options);
}

void PostProcessTree(TRichNodePtr& root, const TCreateTreeOptions& options) {
    PostProcessTreeInt(root, root, options);
}

} // NRichTreeBuilder
