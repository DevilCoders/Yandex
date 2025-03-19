#include "urlcut.h"

#include "cutter.h"
#include "decode_url.h"
#include "cutter_menu.h"
#include "preprocess.h"
#include "tokenizer.h"
#include "tokens.h"
#include "hilited_string.h"

#include <util/charset/wide.h>

namespace NUrlCutter {
    THilitedString HiliteAndCutUrl(const TString& url, size_t maxLen, size_t thresholdLen,
                                   TRichTreeWanderer& rTreeWanderer, ELanguage lang,
                                   ECharset docEncoding, bool cutPrefix, const NSnippets::W2WTrie& domainTrie) {
        TString cutUrl = url;
        TString prefix = CutUrlPrefix(cutUrl);
        PreprocessUrl(cutUrl);
        TUtf16String decodedUrl = DecodeUrlHostAndPath(cutUrl, lang, docEncoding);
        TTokenList tokens = TokenizeAndHilite(decodedUrl, rTreeWanderer, lang, PT_DOMAIN);
        if (!cutPrefix) {
            maxLen -= prefix.length();
        }
        TUrlCutter cutter(tokens, domainTrie, (i32)maxLen, (i32)thresholdLen);
        THilitedString hilitedUrl = cutter.GetUrl();
        if (!cutPrefix) {
            hilitedUrl = THilitedString(ASCIIToWide(prefix)).Append(hilitedUrl);
        }
        return hilitedUrl;
    }

    THilitedString HiliteAndCutUrlMenuHost(const TString& host, size_t maxLen,
                                           TRichTreeWanderer& rTreeWanderer, ELanguage lang,
                                           const NSnippets::W2WTrie& domainTrie) {
        TString cutHost = host;
        CutUrlPrefix(cutHost);
        TUtf16String decodedHost = DecodeUrlHost(cutHost);
        const TUtf16String newHost = domainTrie.GetDefault(decodedHost, u"");
        if (!newHost.empty()) {
            decodedHost = newHost;
        }
        TTokenList tokens = TokenizeAndHilite(decodedHost, rTreeWanderer, lang, PT_DOMAIN);
        TMenuCutter cutter(tokens, (i32)maxLen);
        return cutter.GetUrl();
    }

    THilitedString HiliteAndCutUrlMenuPath(const TString& urlPath, size_t maxLen,
                                           TRichTreeWanderer& rTreeWanderer, ELanguage lang) {
        TString processedUrlPath = urlPath;
        PreprocessUrl(processedUrlPath);
        TUtf16String decodedUrl = DecodeUrlPath(processedUrlPath, lang);
        TTokenList tokens = TokenizeAndHilite(decodedUrl, rTreeWanderer, lang, PT_PATH);
        TMenuCutter cutter(tokens, (i32)maxLen);
        return cutter.GetUrl();
    }
}
