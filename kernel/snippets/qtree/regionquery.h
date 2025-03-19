#pragma once

#include <library/cpp/langmask/langmask.h>

#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

class TRichRequestNode;

namespace NSnippets {

class TRegionQuery : private TNonCopyable {
public:
    TLangMask LangMask;
    int PosCount = 0;
    THashMap<TUtf16String, TVector<int>> Form2Positions;
    TRegionQuery(const TRichRequestNode* tree);
};

} // namespace NSnippets
