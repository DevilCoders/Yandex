#pragma once

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/array_ref.h>
#include <library/cpp/charset/doccodes.h>

#include "richnode.h"

namespace NSearchQuery {

struct TSearchAttr {
    TSearchAttr() {};
    TSearchAttr(const TStringBuf key, const TStringBuf value, ECharset encoding = CODES_UTF8);
    TSearchAttr(const TStringBuf key, const ui32 value, ECharset encoding = CODES_UTF8);
    TSearchAttr(const TUtf16String& key, const TUtf16String& value);
    TSearchAttr(const TUtf16String& key, const ui32 value);
    TSearchAttr(const TRichRequestNode& node);

    bool operator ==(const TSearchAttr& attr) const;

    TUtf16String Key;
    TUtf16String Value;
};

TRichNodePtr CreateOrNode();
TRichNodePtr CreateAndNode();
TRichNodePtr CreateFilterNode();
TRichNodePtr CreateAndNotNode();
TRichNodePtr CreateFilterNode(TRichNodePtr&& node);
TRichNodePtr CreateAttrNode(const TSearchAttr& searchAttr, TCompareOper oper = TCompareOper::cmpEQ);
TRichNodePtr CreateAttrIntervalNode(const TStringBuf key, ui32 attrLow, ui32 attrHigh, TCompareOper oper = TCompareOper::cmpEQ);
TRichNodePtr CreateEmptyAttrNode();
TRichNodePtr CreateWordNode(const TUtf16String& word);
TRichNodePtr CreateOrGroupNode(const TArrayRef<const TRichNodePtr> attrs);
TRichNodePtr CreateOrGroupNode(const TArrayRef<const TSearchAttr> attrs);
TRichNodePtr CreateAndGroupNode(const TArrayRef<const TRichNodePtr> attrs);
TRichNodePtr CreateAndGroupNode(const TArrayRef<const TSearchAttr> attrs);
TRichNodePtr CreateFilterGroupNode(const TArrayRef<const TRichNodePtr> filters);
TRichNodePtr CreateFilterGroupNode(const TArrayRef<const TSearchAttr> filters);
TRichNodePtr CreateFilterGroupNode(const TArrayRef<const TVector<TSearchAttr>> filters);

} // namespace NSearchQuery

template <>
struct THash<NSearchQuery::TSearchAttr> {
    inline size_t operator()(const NSearchQuery::TSearchAttr& attr) const {
        return ComputeHash(attr.Key) + ComputeHash(attr.Value);
    }
};

template<>
inline void Out<NSearchQuery::TSearchAttr> (IOutputStream& stream, const NSearchQuery::TSearchAttr& attr) {
    stream << attr.Key << ":\"" << attr.Value << '\"';
}
