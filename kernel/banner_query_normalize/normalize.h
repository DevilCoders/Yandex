#pragma once

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/printrichnode.h>
#include <kernel/qtree/request/request.h>
#include <kernel/qtree/request/req_node.h>

namespace NBannerQueryNormalize {
    TVector<TString> GetFormsList(TString text);
    void TextNormalize(TUtf16String& wtext);
    TVector<TString> Forms(TUtf16String wtext);
    TRequestNode* GetTree(const TUtf16String& wtext);
    TVector<TString> GoodForms(const TRequestNode* tree);
    void CollectGoodForms2(const TRequestNode* n, TVector<TUtf16String>& words);
    void CollectGoodForms2Impl(const TRichNodePtr& n, TVector<TUtf16String>& words);
}
