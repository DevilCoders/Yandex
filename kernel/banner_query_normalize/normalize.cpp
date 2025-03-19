#include "normalize.h"

#include <library/cpp/tokenizer/tokenizer.h>

using namespace NBannerQueryNormalize;

namespace {
    template <typename T>
    inline bool TransformString(TUtf16String& wtext, T&& f) {
        bool isModified = false;
        TUtf16String::iterator it = wtext.begin();
        for (; it != wtext.end(); ++it) {
            wchar_t nc = f(*it);
            if (nc != *it) {
                *it = nc;
                isModified = true;
            }
        }
        return isModified;
    }
}

TVector<TString> NBannerQueryNormalize::GetFormsList(TString text) {
    TUtf16String wtext = UTF8ToWide(text);
    TextNormalize(wtext);

    TVector<TString> result;
    for (const auto& frm : Forms(wtext)) {
        result.push_back(frm);
    }

    return result;
}

void NBannerQueryNormalize::TextNormalize(TUtf16String& wtext) {
    TransformString(wtext, [](wchar_t c) {
        switch (c) {
            case (L'Ё'):
                return L'е';
            case (L'ё'):
                return L'е';
        }
        return c;
    });
    wtext.to_lower();
}

TVector<TString> NBannerQueryNormalize::Forms(TUtf16String wtext) {
    TRequestNode* tree = GetTree(wtext);

    if (!tree && !wtext.empty()) {
        TransformString(wtext, [](wchar_t c) {
            return c == L'(' || c == L')' // WIZARD-5952
                       ? L' '
                       : c;
        });
        tree = GetTree(wtext);
    }

    if (tree) {
        TVector<TString> result = GoodForms(tree);
        delete tree;
        return result;
    }

    return TVector<TString>();
}

TRequestNode* NBannerQueryNormalize::GetTree(const TUtf16String& wtext) {
    try {
        return tRequest().Analyze(wtext.data()).Release();
    } catch (...) {
        return (TRequestNode*)nullptr;
    }
}

TVector<TString> NBannerQueryNormalize::GoodForms(const TRequestNode* tree) {
    TVector<TUtf16String> words;
    TVector<TString> result;
    words.reserve(10);
    CollectGoodForms2(tree, words);
    for (TVector<TUtf16String>::const_iterator i = words.begin(); i != words.end(); ++i) {
        if (i->size() == 1) {
            wchar32 c32 = ReadSymbol(i->begin(), i->end());
            //drop "special keys" - one-symbol tokens with unicode emoji, math symbols, etc.
            if (IsSpecialTokenizerSymbol(c32)) {
                continue;
            }
        }
        TString utf = WideToUTF8(*i);
        result.push_back(utf);
    }
    return result;
}

void NBannerQueryNormalize::CollectGoodForms2(const TRequestNode* n, TVector<TUtf16String>& words) {
    if (!n) {
        return;
    }

    TCreateTreeOptions options(LI_DEFAULT_REQUEST_LANGUAGES);
    TRichTreePtr tree = CreateRichTreeFromBinaryTree(n, options, NULL, NULL);

    if (!tree.Get() || !tree->Root) {
        return;
    }

    SimpleUnMarkRichTree(tree->Root);
    CollectGoodForms2Impl(tree->Root, words);
}

void NBannerQueryNormalize::CollectGoodForms2Impl(const TRichNodePtr& n, TVector<TUtf16String>& words) {
    if (!n) {
        return;
    }

    if (IsWord(*n)) {
        words.push_back(n->GetText());
    } else {
        for (size_t i = 0; i < n->Children.size(); ++i) {
            CollectGoodForms2Impl(n->Children[i], words);
        }
        if (n->OpInfo.Op == oAndNot) {
            for (size_t i = 0; i < n->MiscOps.size(); ++i) {
                CollectGoodForms2Impl(n->MiscOps[i], words);
            }
        }
    }
}
