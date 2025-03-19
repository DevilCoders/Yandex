#include <kernel/search_types/search_types.h>
#include <util/generic/buffer.h>
#include <util/memory/blob.h>
#include <util/stream/zlib.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <library/cpp/charset/wide.h>
#include <kernel/lemmer/core/langcontext.h>
#include <kernel/lemmer/core/morpho_lang_discr.h>
#include <library/cpp/token/token_structure.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <ysite/yandex/common/prepattr.h>
#include "loadfreq.h"
#include <kernel/qtree/richrequest/protos/rich_tree.pb.h>
#include <kernel/qtree/richrequest/serialization/text_format.h>
#include <kernel/qtree/richrequest/serialization/serializer.h>

#include "lemmas.h"
#include "richnode.h"
#include "printrichnode.h"

#include <cmath>

static NSearchQuery::TRange GetChildRange(const TRichRequestNode* parent, const TRichRequestNode* child, bool allowSyn) {
    if (parent == child) {
        if (parent->Children.empty())
            return NSearchQuery::TRange(0, 0);
        else
            return NSearchQuery::TRange();
    }

    const size_t childPos = parent->Children.FindPosition(child);
    if (childPos != size_t(-1))
        return NSearchQuery::TRange(childPos, childPos);

    if (allowSyn) {
        for (NSearchQuery::TForwardMarkupIterator<TSynonym, true> it(parent->Markup()); !it.AtEnd(); ++it) {
            if (it.GetData().SubTree.Get() == child)
                return it->Range;
        }
    }

    ythrow yexception() << "adulterate child";
}

static size_t GetChildPos(const TRichRequestNode* parent, const TRichRequestNode* child) {
    size_t pos = size_t(-1);
    if (parent != child) {
        pos = parent->Children.FindPosition(child);
        if (pos == size_t(-1))
            ythrow yexception() << "adulterate child";
    } else if (parent->Children.empty()) {
        pos = 0;
    }
    return pos;
}

using namespace NSearchQuery;

const TProximity TRichRequestNode::DisjunctionProxy(0, 0, DOC_LEVEL);

static const char* const LegacyProtocolErrorMessage = "Legacy protocol was deleted. ARC-1032.";

ui64 TRichRequestNode::GetExpectedDocumentsCount() const {
    Y_VERIFY(!!StatInfo);
    if (!StatInfo->GetGlobalDocsCount())
        return 0;
    ui64 result = 0;
    switch (Op()) {
    case oAnd:
    case oRestrDoc:
    case oAndNot: {
        result = StatInfo->GetGlobalDocsCount();
        for (ui32 i = 0; i < Children.size(); ++i) {
            double p = 1.0 * Children[i]->GetExpectedDocumentsCount() / StatInfo->GetGlobalDocsCount();
            if (p < 1)
                result *= p;
        }
        if (oAndNot == Op())
            result = StatInfo->GetGlobalDocsCount() - result;
        return result;
    }
    case oOr:
    case oWeakOr:
    case oZone:
    {
        result = 0;
        for (ui32 i = 0; i < Children.size(); ++i) {
            result += Children[i]->GetExpectedDocumentsCount();
        }
        return result;
    }
    case oRefine: {
        result = 0;
        if (Children.size()) {
            result += Children[0]->GetExpectedDocumentsCount();
        }
        return result;
    }
    default:
    {
        return 0.5 * StatInfo->GetHitsCount();
    }
    }
}

TRichRequestNode::TRichRequestNode(TFormType formType, TWordJoin, TWordJoin)
    : GeoType(GEO_UNK)
    , AttrIntervalRestriction(std::numeric_limits<ui32>::max())
    , HiliteType(HILITE_SELF)
    , SnippetType(SNIPPET_SELF)
    , FormType(formType)
    , Geo(GEO_RULE_NONE)
{
    StatInfo = new TStatInfo();
}

TRichRequestNode::TRichRequestNode(const TRequestNodeBase& src, TNodeNecessity contextNecessity, TFormType formType = fGeneral)
    : TRequestNodeBase(src)
    , GeoType(GEO_UNK)
    , AttrIntervalRestriction(std::numeric_limits<ui32>::max())
    , HiliteType(HILITE_SELF)
    , SnippetType(SNIPPET_SELF)
    , FormType(formType)
    , Geo(GEO_RULE_NONE)
{
    StatInfo = new TStatInfo();
    if (Necessity == nDEFAULT)
        Necessity = contextNecessity;
}

void TRichRequestNode::Swap(TRichRequestNode& node) {
    TRequestNodeBase::Swap(node);

    ::DoSwap(Id, node.Id);

    ::DoSwap(GeoType, node.GeoType);
    ::DoSwap(AttrIntervalRestriction, node.AttrIntervalRestriction);
    ::DoSwap(HiliteType, node.HiliteType);
    ::DoSwap(SnippetType, node.SnippetType);

    ::DoSwap(FormType, node.FormType);

    ::DoSwap(Geo, node.Geo);

    ::DoSwap(Children, node.Children);
    ::DoSwap(MiscOps, node.MiscOps);
    ::DoSwap(WordInfo, node.WordInfo);
}


void TRichRequestNode::SetStopWordHiliteMode() {
    if (!WordInfo)
        return;

    EStickySide stickiness = STICK_NONE;
    bool stopWord = WordInfo->IsStopWord(stickiness);
    if (!stopWord)
        return;
    if (stickiness == STICK_NONE)
        HiliteType = HILITE_SELF;
    else
        HiliteType = HILITE_LEFT_OR_RIGHT;
}

static inline bool CompareNodes(const TRichNodePtr& a, const TRichNodePtr& b) {
    return !a ? !b : (b.Get() && a->Compare(b.Get()));
}

bool TRichRequestNode::CompareVectors(const TNodesVector& left, const TNodesVector& right) {
    return left.size() == right.size()
        && std::equal(left.begin(), left.end(), right.begin(), CompareNodes);
}

bool TRichRequestNode::Compare(const TRichRequestNode* node) const {
    if (!!WordInfo) {
        if (!!node->WordInfo) {
            if (!WordInfo->Compare(*(node->WordInfo)))
                return false;
        } else
            return false;
    } else if (!!node->WordInfo)
        return false;

    return 1
        && TRequestNodeBase::Compare(*node)
        && (node->Parens == Parens)
//        && (node->GeoType == GeoType)  // not serialized
        && (node->HiliteType == HiliteType)
        && (node->SnippetType == SnippetType)
        && (node->GetFormType() == GetFormType())
        && (TRichRequestNode::CompareVectors(node->MiscOps, MiscOps))
        && (node->Children == Children)     // Markup is compared within Children
        && node->Geo == Geo
    ;
}

void TRichRequestNode::AddMarkup(size_t beg, size_t end, const NSearchQuery::TMarkupDataPtr& data) { // @end is the last node of markup range, same as TRange::End
    if (MT_SYNONYM == data->MarkupType() ) {
        TSynonym* syn = static_cast<TSynonym*>(data.Get());
        if (syn->HasSameRichTree(*this))
            return;
    }
    Children.AddMarkup(beg, end, data);
}


TRichTreePtr DeserializeRichTree(const TBinaryRichTree& buffer, EQtreeDeserializeMode mode) {
    TRichTreePtr ret(new NSearchQuery::TRequest);
    ret->Deserialize(buffer, mode);
    return ret;
}

namespace NSearchQuery {

bool TRequest::Compare(const TRequest* tree) const {
    return Softness == tree->Softness && Root->Compare(tree->Root.Get());
}

TRichTreePtr TRequest::Copy() const {
    TRichTreePtr res(new NSearchQuery::TRequest);
    res->Root = Root->Copy();
    res->Softness = Softness;
    return res;
}

void TRequest::Deserialize(const TBinaryRichTree& buffer, EQtreeDeserializeMode mode) {
    TRichTreeDeserializer de;
    de.Deserialize(TStringBuf(buffer.AsCharPtr(), buffer.Size()), *this, mode);
}

void TRequest::Serialize(TBinaryRichTree& blob) const {
    TRichTreeSerializer se;
    TBuffer buffer;
    se.Serialize(*this, buffer);
    blob = TBlob::FromBuffer(buffer);
}

void TRequest::DeserializeFromClearText(const TString& data) {
    bool protobuf = data.substr(0, data.find(':')).find('"') != TString::npos;
    // protobuf text JSON place names into quotes unlike yandex "JSON"
    // { "version":10 } vs { version:10 ... }

    Root = CreateEmptyRichNode();
    if (protobuf) {
        NRichTreeProtocol::TRichRequestNode message;

        if (!NRichTreeProtocol::TextFormat::ParseFromString(data, &message)) {
            ythrow yexception() << "deserialization failed";
        }

        Softness = message.GetBase().GetSoftness();
        Root = ::Deserialize(message);
    } else {
        ythrow yexception() << LegacyProtocolErrorMessage;
    }
}

void TRequest::SerializeToClearText(TString& data) const {
    NRichTreeProtocol::TRichRequestNode message;
    message.MutableBase()->SetSoftness(Softness);
    ::Serialize(*Root, message, true);
    if (!NRichTreeProtocol::TextFormat::PrintToString(message, &data)) {
        ythrow yexception() << "serialization failed";
    }
}
TString TRequest::ToJson() const {
    TStringStream res;
    NRichTreeProtocol::TRichRequestNode message;
    message.MutableBase()->SetSoftness(Softness);
    ::Serialize(*Root, message, true);
    message.PrintJSON(res);
    return res.Str();
}

} // NSearchQuery

//-------------------------------------------------------------------------------

TCreateTreeOptions::TCreateTreeOptions(const TLanguageContext& lang, ui32 reqflags)
    : Lang(lang)
    , Reqflags(reqflags)
    , AttrListTReq(nullptr)
    , AttrListChar(nullptr)
    , MakeSynonymsForMultitokens(true)
    , GenerateForms(true)
{
}

TCreateTreeOptions::TCreateTreeOptions(const TLangMask& mask)
    : Lang(mask)
    , Reqflags(0)
    , AttrListTReq(nullptr)
    , AttrListChar(nullptr)
    , MakeSynonymsForMultitokens(true)
    , GenerateForms(true)
{
}

TCreateTreeOptions::TCreateTreeOptions(const TLanguageContext& lang, const TReqAttrList* attrList, ui32 reqflags)
    : Lang(lang)
    , Reqflags(reqflags)
    , AttrListTReq(attrList)
    , AttrListChar(nullptr)
    , MakeSynonymsForMultitokens(true)
    , GenerateForms(true)
{
}

TCreateTreeOptions::TCreateTreeOptions(const TLanguageContext& lang, const char* attrList, ui32 reqflags)
    : Lang(lang)
    , Reqflags(reqflags)
    , AttrListTReq(nullptr)
    , AttrListChar(attrList)
    , MakeSynonymsForMultitokens(true)
    , GenerateForms(true)
{
}

//-------------------------------------------------------------------------------------

TRichNodePtr TRichRequestNode::Copy() const
{
    TRichNodePtr copyNode(new TRichRequestNode(*this, Necessity));
    copyNode->FormType = FormType;
    copyNode->HiliteType = HiliteType;
    copyNode->SnippetType = SnippetType;
    copyNode->GeoType = GeoType;
    copyNode->Id = Id;
    copyNode->AttrIntervalRestriction = AttrIntervalRestriction;
    copyNode->StatInfo = StatInfo;
    Children.CopyTo(copyNode->Children);    // Markup is copied here too
    copyNode->Geo = Geo;

    for (size_t i = 0; i < MiscOps.size(); ++i)
        copyNode->MiscOps.push_back(MiscOps[i]->Copy());

    if (!!WordInfo)
        copyNode->WordInfo.Reset(new TWordNode(*WordInfo.Get()));
    return copyNode;
}

bool TRichRequestNode::IsOrdinaryWord() const {
    return (IsWord(*this) && Necessity == nDEFAULT && WordInfo.Get() && WordInfo->GetFormType() == fGeneral && !IsQuoted());
}

bool TRichRequestNode::IsExactWord() const {
    return (IsWord(*this) && Necessity == nDEFAULT && WordInfo.Get() && WordInfo->GetFormType() == fExactWord && !IsQuoted());
}

bool TRichRequestNode::IsAttributeInterval() const {
    if (IsAttribute(*this)) {
        if (!!GetAttrValueHi())
            return true;
        wchar16 valueBuf[MAXKEY_BUF];
        //In fact this just StrFCpy and always return 0 but for future compatability ...
        if (!PrepareLiteral(GetText().data(), valueBuf))
            return false;
        TUtf16String value = valueBuf;
        if (OpInfo.CmpOper == cmpEQ && (!value.size() || (unsigned char)value.at(0) != '\"' || value.size() == 1 || (unsigned char)value.at(value.size() - 1) != '*'))
            return false;
        return true;
    }
    return false;
}

bool TRichRequestNode::IsPhrase(bool checkMiscOps) const {
    bool isPhrase = IsWord(*this)
        || IsQuoted()
        || PHRASE_MARKSEQ == GetPhraseType()
        || PHRASE_PHRASE == GetPhraseType()
        || PHRASE_MULTIWORD == GetPhraseType();
    if (checkMiscOps)
        return (isPhrase && MiscOps.empty()) ;
    return isPhrase;
}

bool TRichRequestNode::IsWeakOr() const {
    return oWeakOr == OpInfo.Op;
}

bool TRichRequestNode::IsBinaryOp() const{
    return (IsAndOp(*this)
        || oOr == OpInfo.Op
        || oWeakOr == OpInfo.Op);
}

bool TRichRequestNode::IsAndNotOp() const{
    return oAndNot == OpInfo.Op;
}

bool TRichRequestNode::IsEqualWord(const TWtringBuf& word) const {
    return IsOrdinaryWord() && (word == WordInfo->GetNormalizedForm());
}

bool TRichRequestNode::IsNumber() const {
    return (WordInfo.Get() && Necessity == nDEFAULT &&
            (GetPhraseType() == PHRASE_FLOAT || WordInfo->IsInteger()));

}

bool TRichRequestNode::IsInteger() const {
    return WordInfo.Get() && Necessity == nDEFAULT && WordInfo->IsInteger();
}

bool TRichRequestNode::IsFloat() const {
    return WordInfo.Get() && Necessity == nDEFAULT && GetPhraseType() == PHRASE_FLOAT;
}

bool TRichRequestNode::IsMark() const {
    return IsWord(*this) && !!WordInfo && Necessity == nDEFAULT &&
        !IsNumber() && !WordInfo->IsLemmerWord();
}

bool TRichRequestNode::IsStopWord() const {
    return (WordInfo.Get() && WordInfo->IsStopWord());
}

bool TRichRequestNode::IsExtension() const {
    return false;
}

bool TRichRequestNode::IsAuxiliaryWord() const {
    if (!IsAttribute(*this))
        return false;
    TString name = WideToASCII(GetTextName());
    if (name == "host")
        return true;
    else if (name == "rhost")
        return true;
    else if (name == "inurl")
        return true;
    else if (name == "domain")
        return true;
    else if (name == "site")
        return true;
    else if (name == "url")
        return true;
    else if (name == "serverurl")
        return true;
    else if (name == "surl")
        return true;
    return false;
}

bool TRichRequestNode::IsRestriction() const {
    return (oRestrictByPos == OpInfo.Op
        || IsZone(*this)
        || oAndNot == OpInfo.Op
        || oRestrDoc == OpInfo.Op
        || oRefine == OpInfo.Op);
}

bool TRichRequestNode::ChildrenJustAuxillary() const {
    if (IsWordOrMultitoken(*this))
        return false;
    if (IsAttribute(*this))
        return IsAuxiliaryWord();
    for (size_t i = 0; i < Children.size(); ++i)
        if (!Children[i]->ChildrenJustAuxillary())
            return false;
    for (size_t i = 0; i < MiscOps.size(); ++i)
        if (!MiscOps[i]->ChildrenJustAuxillary())
            return false;
    return true;
}

template <typename T>
static T ConvertTo(const TString& text) {
    const char* s = text.c_str();
    size_t n = text.size();
    while (n && !IsDigit(s[0])) { // skip prefixes # @ $
        ++s;
        --n;
    }
    while (n && !IsDigit(s[n - 1])) // skip suffixes + ++ or #
        --n;
    return FromString<T>(s, n);
}

double TRichRequestNode::GetNumber() const {
    Y_ASSERT(WordInfo.Get() && (GetPhraseType() == PHRASE_FLOAT || WordInfo->IsInteger())); // see also TRichRequestNode::IsNumber()
    return ConvertTo<double>(WideToChar(GetText(), CODES_YANDEX));
}

int TRichRequestNode::GetInteger() const {
    Y_ASSERT(WordInfo.Get() && WordInfo->IsInteger()); // see also TRichRequestNode::IsInteger()
    return ConvertTo<int>(WideToChar(GetText(), CODES_YANDEX));
}

TFormType TRichRequestNode::GetFormType() const {
    return !!WordInfo ? WordInfo->GetFormType() : FormType;
}

void TRichRequestNode::SetFormType(TFormType formType) {
    if (!!WordInfo) {
        WordInfo->SetFormType(formType);
        if (ReverseFreq > 0)
            ReverseFreq = -1;
    }

    FormType = formType;
    for (size_t i = 0; i != Children.size(); ++i)
        Children[i]->SetFormType(formType);
    //for (size_t i = 0; i != Synonyms.size(); ++i)
    //    Synonyms[i].SubTree->SetFormType(formType);
}

void TRichRequestNode::SetParens(bool parens) {
    Parens = parens;
}

size_t TRichRequestNode::CalcAttributeIntervalsCount() const {
    size_t result = IsAttributeInterval() ? 1 : 0;
    for (size_t i = 0; i < Children.size(); ++i)
        result += Children[i]->CalcAttributeIntervalsCount();
    for (size_t i = 0; i < MiscOps.size(); ++i)
        result += MiscOps[i]->CalcAttributeIntervalsCount();
    return result;
}

void TRichRequestNode::SetAttrIntervalRestriction(ui32 r) {
    AttrIntervalRestriction = r;
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->SetAttrIntervalRestriction(r);
    for (size_t i = 0; i < MiscOps.size(); ++i)
        MiscOps[i]->SetAttrIntervalRestriction(r);
}

ui32 TRichRequestNode::GetAttrIntervalRestriction() const {
    return AttrIntervalRestriction;
}


size_t TRichRequestNode::GetChildPos(const TRichNodePtr& child) const {
    return ::GetChildPos(this, child.Get());
}

size_t TRichRequestNode::GetChildPos(const TRichRequestNode& child) const {
    return ::GetChildPos(this, &child);
}

NSearchQuery::TRange TRichRequestNode::GetChildRange(const TRichNodePtr& child, bool allowSyn) const {
    return ::GetChildRange(this, child.Get(), allowSyn);
}

NSearchQuery::TRange TRichRequestNode::GetChildRange(const TRichRequestNode& child, bool allowSyn) const {
    return ::GetChildRange(this, &child, allowSyn);
}

void TRichRequestNode::RemoveChildren(size_t frompos, size_t topos) {
    Children.Remove(frompos, topos);
}


void TRichRequestNode::ReplaceChildren(size_t pos, size_t count, const TRichNodePtr& node) {
    Children.Replace(pos, pos + count, node);
}

void TRichRequestNode::ReplaceChild(size_t pos, const TRichNodePtr& node) {
    Y_ASSERT(node.Get());
    Y_ASSERT(pos < Children.size());
    Children.ReplaceSingle(pos, node);
}


namespace {
    class TMarkupPosition {
    public:
        TMarkupPosition(const TRichRequestNode& first, const TRichRequestNode& last)
            : FirstNode(&first)
            , LastNode(&last)
            , Parent(nullptr)
            , Begin(-1)
            , End(-1)
        {
        }

        TRichRequestNode* Find(TRichRequestNode* root, size_t& beg, size_t& end, bool allowSyn) {
            if (!FindFirst(root, allowSyn) || Begin == -1 || End == -1)
                return nullptr;

            if (!Parent) {
                // whole tree
                Parent = root;
                Begin = 0;
                End = root->Children.empty() ? 0 : root->Children.size() - 1;
            }

            beg = Begin;
            end = End;
            return Parent;
        }

    private:
        bool IsLastNode(TRichRequestNode* node, bool allowSyn) const {
            if (node == LastNode)
                return true;
            if (!node->Children.empty() && IsLastNode(node, node->Children.size() - 1, allowSyn, 0))
                return true;

            if (allowSyn) {
                for (TForwardMarkupIterator<TSynonym, false> i(node->MutableMarkup()); !i.AtEnd(); ++i) {
                    if (i->Range.End != 0)
                        continue;
                    if (IsLastNode(i.GetData().SubTree.Get(), allowSyn))
                        return true;
                }
            }
            return false;
        }

        bool IsLastNode(TRichRequestNode* root, size_t childNum, bool allowSyn, size_t firstNode) const {
            Y_ASSERT(root->Children.size() > childNum);
            if (IsLastNode(root->Children[childNum].Get(), allowSyn))
                return true;

            if (allowSyn) {
                for (TForwardMarkupIterator<TSynonym, false> i(root->MutableMarkup()); !i.AtEnd(); ++i) {
                    if (i->Range.End != childNum || i->Range.Beg < firstNode)
                        continue;
                    if (IsLastNode(i.GetData().SubTree.Get(), allowSyn))
                        return true;
                }
            }
            return false;
        }

        bool FindLastNode(TRichRequestNode* root, size_t startWith, size_t firstNode, bool allowSyn) {
            Y_ASSERT(!root->Children.empty());
            for (size_t j = startWith; j < root->Children.size(); ++j) {
                if (IsLastNode(root, j, allowSyn, firstNode)) {
                    End = j;    // LastNode is here
                    Parent = root;
                    return true;
                }
            }
            // root can be LastNode too
            if (firstNode == 0 && LastNode == root) {
                End = root->Children.size() - 1;
                return true;
            }
            return false;
        }

        // Possible outcomes:
        // 1. root is FirstNode
        // 2. FirstNode is on the left side of sub-tree
        // 3. FirstNode is here, but not on the left side (fail)
        // 4. both FirstNode and LastNode in this sub-tree
        // 5. FirstNode is not here
        bool FindFirst(TRichRequestNode* root, bool allowSyn) {
            Y_ASSERT(Begin == -1);       // FirstNode is not found yet
            if (root == FirstNode) {
                Begin = 0;              // root is FirstNode
                if (IsLastNode(root, allowSyn))   // and is LastNode at same time
                    End = 0;
                return true;
            }
            for (size_t i = 0; i < root->Children.size(); ++i) {
                if (!FindFirst(root->Children[i].Get(), allowSyn))
                    return false;

                if (Begin != -1 && End != -1) {
                    if (!Parent) {
                        Parent = root;
                        Begin = End = i;
                    }
                    return true;
                }

                if (Begin != -1) {
                    Begin = i;      // FirstNode is here
                    if (FindLastNode(root, i, i, allowSyn))
                        return true;
                    return Begin == 0;  // FirstNode is here (should be on the left side of sub-tree), LastNode is not here
                }
            }
            if (Begin == -1 && allowSyn) {
                for (TForwardMarkupIterator<TSynonym, false> i(root->MutableMarkup()); !i.AtEnd(); ++i) {
                    if (!FindFirst(i.GetData().SubTree.Get(), allowSyn))
                        return false;
                    if (Begin != -1 && End != -1) {
                        if (!Parent) {
                            Parent = i.GetData().SubTree.Get();
                            Begin = 0;
                            End = Parent->Children.empty() ? 0 : Parent->Children.size() - 1 ;
                        }
                        return true;
                    }
                    if (Begin != -1) {
                        Begin = i->Range.Beg;
                        if (root->Children.empty()) {
                            if (IsLastNode(root, allowSyn)) {
                                End = 0;
                                return true;
                            }
                        } else {
                            if (FindLastNode(root, i->Range.End, i->Range.Beg, allowSyn))
                                return true;
                        }
                        return Begin == 0;
                    }
                }
            }

            return true;    // FirstNode is not in this sub-tree
        }

    private:
        const TRichRequestNode* FirstNode;
        const TRichRequestNode* LastNode;

        TRichRequestNode* Parent;
        int Begin;      // -1 means "not found yet"
        int End;
    };
}   // namespace


TRichRequestNode* TRichRequestNode::FindMarkupPosition(const TRichRequestNode& first, const TRichRequestNode& last,
                                                       size_t& beg, size_t& end, bool allowSyn)
{
    TMarkupPosition pos(first, last);
    return pos.Find(this, beg, end, allowSyn);
}

bool TRichRequestNode::AddMarkupDeep(const TRichRequestNode& firstNode, const TRichRequestNode& lastNode,
                                     const NSearchQuery::TMarkupDataPtr& data, bool allowSyn) {
    size_t beg = 0, end = 0;
    TRichRequestNode* parent = FindMarkupPosition(firstNode, lastNode, beg, end, allowSyn);
    if (!parent)
        return false;

    parent->AddMarkup(beg, end, data);
    return true;
}


static void AddSynonymImpl(TRichRequestNode* parent, size_t beg, size_t end, TAutoPtr<TSynonym> synonym);

static inline bool TrySplitMultitokenSynonym(TRichRequestNode* parent, TSynonym* synonym) {
    if (::IsMultitoken(*parent) && parent->Children.size() == synonym->SubTree->Children.size()) {
        for (size_t i = 0; i < parent->Children.size(); ++i)
            AddSynonymImpl(parent, i, i, synonym->SplitChild(i));
        return true;
    }
    return false;
}

static void ModifyAllSynonymsType(TRichRequestNode& tree, TThesaurusExtensionId Addtype) {
    for (TForwardMarkupIterator<TSynonym, false> i(tree.MutableMarkup()); !i.AtEnd(); ++i) {
        ModifyAllSynonymsType(*i.GetData().SubTree, Addtype);
        i.GetData().AddType(Addtype);
    }
    for (TForwardMarkupIterator<TTechnicalSynonym, false> i(tree.MutableMarkup()); !i.AtEnd(); ++i) {
        ModifyAllSynonymsType(*i.GetData().SubTree, Addtype);
        i.GetData().AddType(Addtype);
    }

}

static void AddSynonymImpl(TRichRequestNode* parent, size_t beg, size_t end, TAutoPtr<TSynonym> synonym) {
    // @parent, @beg, @end should be correct here (e.g. produced by FindMarkupPosition)

    // try to split @synonym with the same tree structure across sub-nodes
    if (!parent->Children.empty() && !synonym->SubTree->Children.empty()) {
        size_t synLen = synonym->SubTree->Children.size();

        // always split multitokens
        if (::IsMultitoken(*synonym->SubTree)) {
            if (beg == 0 && end + 1 == synLen && TrySplitMultitokenSynonym(parent, synonym.Get()))
                return;
            if (beg == end && TrySplitMultitokenSynonym(parent->Children[beg].Get(), synonym.Get()))
                return;
        }
    }
    ModifyAllSynonymsType(*synonym->SubTree, synonym->GetType());
    parent->AddMarkup(beg, end, synonym.Release());
}


bool TRichRequestNode::AddSynonym(const TRichRequestNode& firstNode, const TRichRequestNode& lastNode, TAutoPtr<TSynonym> synonym, bool allowSyn) {
    size_t beg = 0, end = 0;
    TRichRequestNode* parent = FindMarkupPosition(firstNode, lastNode, beg, end, allowSyn);
    if (!parent)
        return false;

    AddSynonymImpl(parent, beg, end, synonym);
    return true;
}


void TRichRequestNode::InsertChild(size_t pos, TRichNodePtr node, const TProximity& distance /*distance to preceding node*/) {
    Children.Insert(pos, node, distance);
}

void TRichRequestNode::AddZoneFilter(const TString& zoneName) {
    Y_ASSERT(!!zoneName);
    TRichNodePtr node(CreateEmptyRichNode());
    node->SetZoneName(UTF8ToWide(zoneName), false);
    node->OpInfo = DefaultZoneOpInfo;
    MiscOps.push_back(node);
}

static inline TRichNodePtr CreateDocFilter(const TRichNodePtr& restriction, const TOpInfo& operation) {
    Y_ASSERT(!!restriction);
    TRichNodePtr node = CreateEmptyRichNode();
    node->Children.Append(restriction);
    node->OpInfo = operation;
    return node;
}

void TRichRequestNode::AddRefineFilter(const TRichNodePtr& restriction) {
    MiscOps.push_back(CreateDocFilter(restriction, DefaultRefineOpInfo));
}

void TRichRequestNode::AddRefineFactorFilter(const TRichNodePtr& restriction, const TStringBuf& factorName, float factorVal) {
    TRichNodePtr node = CreateDocFilter(restriction, DefaultRefineOpInfo);
    node->SetRefineFactor(UTF8ToWide(factorName), factorVal);
    MiscOps.push_back(node);
}

void TRichRequestNode::AddRestrDocFilter(const TRichNodePtr& restriction) {
    MiscOps.push_back(CreateDocFilter(restriction, DefaultRestrDocOpInfo));
}

void TRichRequestNode::AddAndNotFilter(const TRichNodePtr& restriction) {
    MiscOps.push_back(CreateDocFilter(restriction, DefaultAndNotOpInfo));
}

void TRichRequestNode::AddRestrictByPosFilter(const TRichNodePtr& restriction) {
    Y_ASSERT(!!restriction);
    //Won't work correctly: Lo == 0 and Hi == 0.
    restriction->OpInfo = DefaultRestrictByPosOpInfo;
    MiscOps.push_back(restriction);
}

bool TRichRequestNode::HasExtension(const TRichNodePtr& node, TThesaurusExtensionId type) {
    const TVector<TMarkupItem>& synonyms = node->Markup().GetItems<TSynonym>();

    for (size_t i = 0 ; i < synonyms.size() ; ++i) {
        if (synonyms[i].GetDataAs<TSynonym>().HasType (type))
            return true;
    }

    for (size_t i = 0 ; i != node->Children.size() ; ++i)
        if (HasExtension(node->Children[i], type))
            return true;

    return false;
}

TUtf16String TRichRequestNode::Print(EPrintRichRequestOptions format) const {
    return PrintRichRequest(*this, format);
}

static void DebugPrint(const TRichRequestNode& node, IOutputStream& out) {
    out << "\"" << node.GetText() << "\"";
    if (node.Children.size()) {
        out << " -> [";
        for (size_t i = 0; i < node.Children.size(); ++i) {
            if (i > 0)
                out << ", ";
            DebugPrint(*node.Children[i], out);
        }
        out << "]";
    }
}

TString TRichRequestNode::DebugString() const {
    TStringStream out;
    DebugPrint(*this, out);
    return out.Str();
}

EGeoType TRichRequestNode::GetGeoType() const {
    return GeoType;
}

void TRichRequestNode::SetGeoType(EGeoType type) {
    GeoType = type;
}

bool TRichRequestNode::HasGeo() const {
    return Geo != 0;
}

bool TRichRequestNode::HasGeoRuleType(EGeoRuleType type) const {
    return (Geo & type);
}

bool TRichRequestNode::HasGeoAddr() const {
    return HasGeoRuleType(GEO_RULE_ADDR);
}
bool TRichRequestNode::HasGeoCity() const {
    return HasGeoRuleType(GEO_RULE_CITY);
}

void TRichRequestNode::SetGeoRuleType(EGeoRuleType type) {
    Geo = static_cast<EGeoRuleType>(Geo | type);
}

EGeoRuleType TRichRequestNode::GetGeoRuleType() const {
    return Geo;
}

void TRichRequestNode::SetGeoAddr() {
    SetGeoRuleType(GEO_RULE_ADDR);
}

void TRichRequestNode::SetGeoCity() {
    SetGeoRuleType(GEO_RULE_CITY);
}

TRichRequestNode::TId TRichRequestNode::GetId() const {
    return static_cast<TRichRequestNode::TId>(Id.GetId());
}

EHiliteType TRichRequestNode::GetHiliteType() const {
    return HiliteType;
}

void TRichRequestNode::SetHiliteType(EHiliteType type) {
    HiliteType = type;
}

ESnippetType TRichRequestNode::GetSnippetType() const {
    return SnippetType;
}

void TRichRequestNode::SetSnippetType(ESnippetType type) {
    SnippetType = type;
}

TString EncodeRichTreeBase64(const TBinaryRichTree& tree, bool url) {
    const TStringBuf str((const char*)tree.AsUnsignedCharPtr(), tree.Size());

    if (url) {
        return Base64EncodeUrl(str);
    } else {
        return Base64Encode(str);
    }
}

TBinaryRichTree DecodeRichTreeBase64(const TStringBuf& str) {
    return TBlob::FromString(Base64Decode(str));
}

TString CreatePackedTree(const TUtf16String& query, const TCreateTreeOptions& treeOpt, const TPure* pure) {
    TRichTreePtr richtree(CreateRichTree(query, treeOpt));
    if (Y_LIKELY(nullptr != pure))
        LoadFreq(*pure, *richtree->Root);
    UpdateRichTree(richtree->Root);
    TBinaryRichTree serialized;
    richtree->Serialize(serialized);
    return EncodeRichTreeBase64(serialized, true);
}

void TRichRequestNode::MakeNoBreakLevelDistance(bool convert) {
    if (convert) {
        OpInfo.Lo = 0;
        OpInfo.Hi = 0;
        OpInfo.Level = 1;
        for (size_t i = 1; i < Children.size(); ++i) {
            if (Children.ProxBefore(i).Level < DOC_LEVEL) {
        // break-level distances should be preserved
        // if they come from an explicit operator
                if (Children.ProxBefore(i).DistanceType == DT_USEROP ||
                    Children.ProxBefore(i).DistanceType == DT_QUOTE )
                    continue;

                // or from a multitoken with digits
                if (Children.ProxBefore(i).DistanceType == DT_MULTITOKEN &&
                    (Children[i-1]->IsNumber() || Children[i]->IsNumber()))
                    continue;
        }

            Children.ProxBefore(i) = TProximity(0, 0, 1, Children.ProxBefore(i).DistanceType);
        }
    }
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->MakeNoBreakLevelDistance(true);
    for (TForwardMarkupIterator<TSynonym, false> i(MutableMarkup()); !i.AtEnd(); ++i)
        i.GetData().SubTree->MakeNoBreakLevelDistance(true);
    for (TForwardMarkupIterator<TTechnicalSynonym, false> i(MutableMarkup()); !i.AtEnd(); ++i)
        i.GetData().SubTree->MakeNoBreakLevelDistance(true);
    for (size_t i = 0; i < MiscOps.size(); ++i)
        MiscOps[i]->MakeNoBreakLevelDistance(false);
}

void TRichRequestNode::MakeSoftDistances(bool keepUserDistance) {
    for (size_t i = 1; i < Children.size(); ++i) {
        TProximity& prox = Children.ProxBefore(i);
        if (prox.DistanceType == DT_MULTITOKEN || prox.DistanceType == DT_PHRASE || (!keepUserDistance && prox.DistanceType == DT_USEROP))
            prox.DistanceType = DT_SYNTAX;
    }
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->MakeSoftDistances(keepUserDistance);
    for (TForwardMarkupIterator<TSynonym, false> i(MutableMarkup()); !i.AtEnd(); ++i)
        i.GetData().SubTree->MakeSoftDistances(keepUserDistance);
    for (size_t i = 0; i < MiscOps.size(); ++i)
        MiscOps[i]->MakeSoftDistances(keepUserDistance);
}

bool TRichRequestNode::HasQueryLanguage() const {
    for (size_t i = 0; i < Children.size(); ++i)
        if (Children[i]->HasQueryLanguage())
            return true;

    for (TForwardMarkupIterator<TSynonym, true> i(Markup()); !i.AtEnd(); ++i)
        if (i.GetData().SubTree->HasQueryLanguage())
            return true;
    return !MiscOps.empty();
}

bool TRichRequestNode::HasAnyLemma(const THashSet<TUtf16String>& lemmas) const {
    TIntersectionChecker c(lemmas);
    c.CollectLemmas(this);
    return c.Intersects();
}

void TRichRequestNode::CollectLemmas(TVector<TUtf16String>& lemmas) const {
    TVectorLemmaCollector c(lemmas);
    c.CollectLemmas(this);
    FilterUniqItems(lemmas);
}

void TRichRequestNode::CollectLemmas(THashSet<TUtf16String>& lemmas) const {
    THashSetLemmaCollector c(lemmas);
    c.CollectLemmas(this);
}

void TRichRequestNode::CollectForms(TVector<TUtf16String>& forms) const {
    TVectorFormsCollector c(forms);
    c.CollectForms(this);
    FilterUniqItems(forms);
}

void TRichRequestNode::UpdateKeyPrefix(const TVector<ui64>& value) {
    SetKeyPrefix(value);

    for (TForwardMarkupIterator<TSynonym, true> i(MutableMarkup()); !i.AtEnd(); ++i) {
        const TSynonym &syn = i.GetData();
        if (!!syn.SubTree)
            syn.SubTree->UpdateKeyPrefix(value);
    }

    for (size_t i = 0; i < Children.size(); ++i) {
        Children[i]->UpdateKeyPrefix(value);
    }

    for (size_t i = 0; i < MiscOps.size(); ++i) {
        MiscOps[i]->UpdateKeyPrefix(value);
    }
}

ui32 CalcWordCount(const TRichRequestNode& node, bool nonStop) {
    if (IsAndOp(node)) {
        ui32 result = 0;
        for (size_t i = 0; i < node.Children.size(); ++i) {
            const TRichRequestNode* p = node.Children[i].Get();
            if (IsAndOp(*p) || p->Op() == oWeakOr || p->Op() == oOr || IsWord(*p) || IsAttribute(*p)) {
                if (!nonStop) {
                    ++result;
                } else {
                    if (!p->IsStopWord() || (p->Necessity > 0))
                        ++result;
                }
            }
        }
        return result;
    } else {
        return 1;
    }
}
