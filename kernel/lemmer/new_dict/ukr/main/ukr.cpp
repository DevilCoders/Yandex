#include "ukr.h"

#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/core/rubbishdump.h>
#include <library/cpp/tokenizer/nlpparser.h>

NLemmer::TLllLevel TUkrLanguage::LooksLikeLemma(const TYandexLemma& lemma) const {
    return NLanguageRubbishDump::EasternSlavicLooksLikeLemma(lemma);
}

TAutoPtr<NLemmer::TGrammarFiltr> TUkrLanguage::LooksLikeLemmaExt(const TYandexLemma& lemma) const {
    return NLanguageRubbishDump::EasternSlavicLooksLikeLemmaExt(this, lemma);
}

namespace {
    typedef std::pair<const wchar16*, size_t> TTcpI;
}

static TTcpI UkrainianHardConsonants() {
    static const wchar16 hc[] = {
        0x431, 0x432, 0x433, 0x434, 0x436, 0x437, 0x43A, 0x43C, 0x43D, 0x43F, 0x440, 0x441, 0x442, 0x444, 0x445, 0x448,
        // b,   v,    g,    d,    zh,   z,    k,    m,    n,    p,    r,    s,    t,    f,   kh,   sh
        0x411, 0x412, 0x413, 0x414, 0x416, 0x417, 0x41A, 0x41C, 0x41D, 0x41F, 0x420, 0x421, 0x422, 0x424, 0x425, 0x428,
        // b,   v,    g,    d,    zh,   z,    k,    m,    n,    p,    r,    s,    t,    f,   kh,   sh
        0};
    static const size_t len = sizeof(hc) / sizeof(wchar16) - 1;
    return TTcpI(hc, len);
}

static TTcpI UkrainianIotatedVowels() {
    static const wchar16 iv[] = {
        0x44F, 0x454, 0x457, 0x44E,
        // ja,   je,   ji,   ju,
        0x42F, 0x404, 0x407, 0x42E,
        // ja,   je,   ji,   ju,
        0};
    static const size_t len = sizeof(iv) / sizeof(wchar16) - 1;
    return TTcpI(iv, len);
}

bool TUkrLanguage::CanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t /*len2*/, bool /*isTranslit*/) const {
    if (pos1 + len1 + 1 != pos2)
        return true;
    if (TNlpParser::GetCharClass(word[pos1 + len1]) != TNlpParser::CC_APOSTROPHE)
        return true;

    TTcpI iv = UkrainianIotatedVowels();
    if (!TWtringBuf{iv.first, iv.second}.Contains(word[pos2]))
        return true;
    TTcpI hc = UkrainianHardConsonants();
    if (!TWtringBuf{hc.first, hc.second}.Contains(word[pos1 + len1 - 1]))
        return true;

    return false;
}
