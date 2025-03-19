#include "util.h"
#include "kernel/qtree/request/nodebase.h"

#include <library/cpp/charset/wide.h>


namespace {

TRichNodePtr CreateBinaryNode(const TOpInfo& opInfo)
{
    TRichNodePtr node(CreateEmptyRichNode());
    node->SetPhrase(PHRASE_PHRASE);
    node->Parens = true;
    node->OpInfo = opInfo;
    return node;
}

}

namespace NSearchQuery {

TSearchAttr::TSearchAttr(const TStringBuf key, const TStringBuf value, ECharset encoding)
    : Key(CharToWide(key, encoding))
    , Value(CharToWide(TString::Join("\"", value), encoding))
{}

TSearchAttr::TSearchAttr(const TStringBuf key, const ui32 value, ECharset encoding)
    : Key(CharToWide(key, encoding))
    , Value(UTF8ToWide(ToString(value)))
{}

TSearchAttr::TSearchAttr(const TUtf16String& key, const TUtf16String& value)
    : Key(key)
    , Value(u"\"" + value)
{}

TSearchAttr::TSearchAttr(const TUtf16String& key, const ui32 value)
    : Key(key)
    , Value(UTF8ToWide(ToString(value)))
{}

TSearchAttr::TSearchAttr(const TRichRequestNode& node)
    : Key(node.GetTextName())
    , Value(node.GetText())
{}

bool TSearchAttr::operator ==(const TSearchAttr& attr) const
{
    return Key == attr.Key && Value == attr.Value;
}

TRichNodePtr CreateOrNode()
{
    return CreateBinaryNode(DisjunctionOpInfo);
}

TRichNodePtr CreateAndNode()
{
    return CreateBinaryNode(DefaultPhraseOpInfo);
}

TRichNodePtr CreateFilterNode()
{
    return CreateBinaryNode(DefaultRestrDocOpInfo);
}

TRichNodePtr CreateFilterNode(TRichNodePtr&& node)
{
    TRichNodePtr filterNode = CreateFilterNode();
    filterNode->Children.Append(std::move(node));
    return filterNode;
}

TRichNodePtr CreateAndNotNode() {
    return CreateBinaryNode(DefaultAndNotOpInfo);
}

TRichNodePtr CreateAttrNodeInternal(const TSearchAttr& attr, const TUtf16String& attrHigh, TCompareOper oper) {
    TRichNodePtr attrNode(CreateEmptyRichNode());
    attrNode->SetAttrValues(attr.Key, attr.Value, attrHigh);
    attrNode->OpInfo.CmpOper = oper;
    attrNode->WordInfo.Reset(TWordNode::CreateEmptyNode().Release());
    return attrNode;
}

TRichNodePtr CreateAttrNode(const TSearchAttr& attr, TCompareOper oper)
{
    return CreateAttrNodeInternal(attr, u"", oper);
}

TRichNodePtr CreateAttrIntervalNode(const TStringBuf key, ui32 attrLow, ui32 attrHigh, TCompareOper oper) {
    TUtf16String attrHighValue = UTF8ToWide(ToString(attrHigh));
    return CreateAttrNodeInternal({key, attrLow}, attrHighValue, oper);
}

TRichNodePtr CreateEmptyAttrNode()
{
    return CreateAttrNode(TSearchAttr("emptyattr", "emptyvalue"));
}

TRichNodePtr CreateWordNode(const TUtf16String& word)
{
    return CreateRichNode(word, TCreateTreeOptions(LI_DEFAULT_REQUEST_LANGUAGES));
}

TRichNodePtr CreateOrGroupNode(const TArrayRef<const TRichNodePtr> attrs)
{
    Y_ASSERT(!attrs.empty());
    if (attrs.size() == 1) {
        return attrs.front();
    }

    TRichNodePtr groupNode = CreateOrNode();
    for (size_t i = 0; i < attrs.size(); ++i) {
        groupNode->Children.Append(attrs[i], i == 0 ? TProximity() : TRichRequestNode::DisjunctionProxy);
    }
    return groupNode;
}

TRichNodePtr CreateOrGroupNode(const TArrayRef<const TSearchAttr> attrs)
{
    TVector<TRichNodePtr> tmpNodes;
    tmpNodes.reserve(attrs.size());
    Transform(attrs.begin(), attrs.end(), std::back_inserter(tmpNodes),
        [](const auto& attr) { return CreateAttrNode(attr); });
    return CreateOrGroupNode(tmpNodes);
}

TRichNodePtr CreateAndGroupNode(const TArrayRef<const TRichNodePtr> attrs)
{
    Y_ASSERT(!attrs.empty());
    if (attrs.size() == 1) {
        return attrs.front();
    }

    TRichNodePtr groupNode = CreateAndNode();
    for (size_t i = 0; i < attrs.size(); ++i) {
        groupNode->Children.Append(attrs[i]);
    }
    return groupNode;
}

TRichNodePtr CreateAndGroupNode(const TArrayRef<const TSearchAttr> attrs)
{
    TVector<TRichNodePtr> tmpNodes;
    tmpNodes.reserve(attrs.size());
    Transform(attrs.begin(), attrs.end(), std::back_inserter(tmpNodes),
        [](const auto& attr) { return CreateAttrNode(attr); });
    return CreateAndGroupNode(tmpNodes);
}

TRichNodePtr CreateFilterGroupNode(const TArrayRef<const TVector<TSearchAttr>> filters)
{
    TVector<TRichNodePtr> andFilters;
    andFilters.reserve(filters.size());
    Transform(filters.begin(), filters.end(), std::back_inserter(andFilters),
        [](const auto& attrs) { return CreateAndGroupNode(attrs); });
    return CreateFilterGroupNode(andFilters);
}

TRichNodePtr CreateFilterGroupNode(const TArrayRef<const TRichNodePtr> filters)
{
    return CreateFilterNode(CreateOrGroupNode(filters));
}

TRichNodePtr CreateFilterGroupNode(const TArrayRef<const TSearchAttr> filters)
{
    return CreateFilterNode(CreateOrGroupNode(filters));
}
} // namespace NSearchQuery
