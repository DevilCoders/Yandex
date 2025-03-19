#include "transmap.h"

void NTranslit::TableTranslitBySymbol(const TWtringBuf& src, TString& dst, const TCharTable& charTable) {
    dst.clear();
    if (src.empty())
        return;

    size_t n = src.size();
    dst.resize(n * 1.1);
    size_t dstpos = 0;
    for (size_t i = 0; i < n; ++i) {
        wchar16 c = src[i];
        if (c >= 32 && c <= 126) {
            if (dstpos + 1 > dst.size()) {
                size_t newsize = dst.size() * 2 + 1;
                dst.resize(newsize);
            }
            *(dst.begin() + dstpos) = c;
            ++dstpos;
            continue;
        }
        auto replacement = charTable.find(c);

        if (charTable.find(c) == charTable.end()) {
            continue;
        }

        size_t tsize = replacement->second.size();
        if (dstpos + tsize > dst.size()) {
            size_t newsize = dst.size() * 2 + tsize;
            dst.resize(newsize);
        }
        memcpy(dst.begin() + dstpos, replacement->second.data(), tsize);
        dstpos += tsize;
    }
    Y_ASSERT(dstpos <= dst.size());
    dst.resize(dstpos);
}

void TableTranslitBySymbol(const TWtringBuf& src, ELanguage lang, TString& dst) {
    const NTranslit::TCharTable* table = nullptr;
    if (CyrillicScript(lang)) {
        table = &NTranslit::GetCyrillic();
    } else if (lang == LANG_GRE) {
        table = &NTranslit::GetGreek();
    } else {
        table = &NTranslit::GetLatin();
    }
    NTranslit::TableTranslitBySymbol(src, dst, *table);
}
