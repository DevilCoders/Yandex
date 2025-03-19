#include "cnorm.h"

#include <kernel/qtree/request/reqattrlist.h>

#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/bitops.h>
#include <util/generic/list.h>
#include <util/generic/noncopyable.h>
#include <util/generic/serialized_enum.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/generic/reserve.h>
#include <util/string/split.h>
#include <util/string/join.h>

namespace NCnorm { // Configurable Normalizer

    constexpr wchar32 colon = U':';
    constexpr wchar32 dash = U'-';
    constexpr wchar32 plus = U'+';
    constexpr wchar32 whitespace = U' ';

    class TAttributeSet: TNonCopyable {
    private:
        TReqAttrList Attrs;

    public:
        bool IsAttribute(TUtf32StringBuf attr) const {
            TUtf16String utf16 = UTF32ToWide(attr.data(), attr.length());
            return TReqAttrList::IsAttr(Attrs.GetType(utf16));
        }
        bool IsAttribute(TUtf16String& attr) const {
            return TReqAttrList::IsAttr(Attrs.GetType(attr));
        }
    };

    TUtf32String TConfigurableNormalizer::UnicodeNormalize(const TUtf32StringBuf text) const {
        if (UnicodeNormalizationType.Empty()) {
            return ToUtf32String(text);
        }
        switch (UnicodeNormalizationType.GetRef()) {
            case NUnicode::ENormalization::NFD: {
                return ::Normalize<NUnicode::ENormalization::NFD>(text);
            }
            case NUnicode::ENormalization::NFC: {
                return ::Normalize<NUnicode::ENormalization::NFC>(text);
            }
            case NUnicode::ENormalization::NFKD: {
                return ::Normalize<NUnicode::ENormalization::NFKD>(text);
            }
            case NUnicode::ENormalization::NFKC: {
                return ::Normalize<NUnicode::ENormalization::NFKC>(text);
            }
        }
    }

    void TConfigurableNormalizer::RemoveExtraSymbols(TUtf32String& text) const {
        TUtf32String::iterator begin = text.begin();
        TUtf32String::iterator end = text.vend();
        auto it = std::remove_if(
            begin,
            end,
            [this](const wchar32 symbol) {
                return RemoveConfig.IsAllowedSymbol(symbol);
            });
        text.erase(it, end);
    }

    TList<TUtf32StringBuf> TConfigurableNormalizer::SplitByWhitespaces(const TUtf32StringBuf text) const {
        return StringSplitter(text)
            .SplitByFunc([](const wchar32 symbol) { return IsWhitespace(symbol); })
            .SkipEmpty();
    }

    void TConfigurableNormalizer::RemoveAttributes(TList<TUtf32StringBuf>& tokens) const {
        EraseNodesIf(
            tokens,
            [](const TUtf32StringBuf token) {
                TUtf32StringBuf left, right;
                token.Split(colon, left, right);
                return right && Singleton<TAttributeSet>()->IsAttribute(left);
            });
    }

    void TConfigurableNormalizer::RemoveTrailingDashStartingTokens(TList<TUtf32StringBuf>& tokens) const {
        while ((tokens.size() > 1) && (tokens.back().length() > 1) && (tokens.back()[0] == dash) && IsAlpha(tokens.back()[1])) {
            tokens.pop_back();
        }
    }

    void TConfigurableNormalizer::ProcessSpecialSymbols(TList<TUtf32StringBuf>& tokens) const {
        for (auto it = tokens.begin(); it != tokens.end();) {
            const TUtf32StringBuf token = *it;
            size_t startIdx = 0;
            size_t endIdx = 0;
            for (; endIdx != token.length(); ++endIdx) {
                if (NeedKeepTrailingPlusSymbols && (startIdx != endIdx) && (token[endIdx] == plus)) {
                    size_t lastPlusIdx = endIdx + 1;
                    while ((lastPlusIdx != token.length()) && (token[lastPlusIdx] == plus)) {
                        ++lastPlusIdx;
                    }
                    if ((lastPlusIdx == token.length()) || SeparateConfig.IsAllowedSymbol(token[lastPlusIdx]) ||
                        ReplaceByWhitespaceConfig.IsAllowedSymbol(token[lastPlusIdx]))
                    { // plusses are at the end of a token and we should keep them
                        tokens.emplace(it, token.SubStr(startIdx, lastPlusIdx - startIdx));
                        endIdx = lastPlusIdx - 1;
                        startIdx = endIdx + 1;
                        continue;
                    }
                }
                if (SeparateConfig.IsAllowedSymbol(token[endIdx])) {
                    if (startIdx != endIdx) {
                        tokens.emplace(it, token.SubStr(startIdx, endIdx - startIdx));
                    }
                    tokens.emplace(it, token.SubStr(endIdx, 1));
                    startIdx = endIdx + 1;
                } else if (ReplaceByWhitespaceConfig.IsAllowedSymbol(token[endIdx])) {
                    if (startIdx != endIdx) {
                        tokens.emplace(it, token.SubStr(startIdx, endIdx - startIdx));
                    }
                    startIdx = endIdx + 1;
                }
            }
            if (startIdx) {
                if (startIdx != endIdx) {
                    tokens.emplace(it, token.SubStr(startIdx, endIdx - startIdx));
                }
                tokens.erase(it++);
            } else {
                it++;
            }
        }
    }

    void TConfigurableNormalizer::SplitAlphaNumericTokens(TList<TUtf32StringBuf>& tokens) const {
        for (auto it = tokens.begin(); it != tokens.end();) {
            TUtf32StringBuf token = *it;
            size_t startIdx = 0;
            size_t endIdx = 0;
            for (; endIdx != token.length(); ++endIdx) {
                if ((startIdx != endIdx) &&
                    ((IsAlphabetic(token[endIdx]) && IsNumeric(token[endIdx - 1])) ||
                    (IsNumeric(token[endIdx]) && IsAlphabetic(token[endIdx - 1]))))
                {
                    tokens.emplace(it, token.SubStr(startIdx, endIdx - startIdx));
                    startIdx = endIdx;
                }
            }
            if (startIdx) {
                tokens.emplace(it, token.SubStr(startIdx, endIdx - startIdx));
                tokens.erase(it++);
            } else {
                it++;
            }
        }
    }

    void TConfigurableNormalizer::ApplyLimits(TList<TUtf32StringBuf>& tokens) const {
        if (MaxTokensNumber && (tokens.size() > MaxTokensNumber)) {
            tokens.resize(MaxTokensNumber);
        }
        for (TUtf32StringBuf& token : tokens) {
            if (MaxTokenLength && (token.length() > MaxTokenLength)) {
                token = token.Head(MaxTokenLength);
            }
        }
    }

    void SetMaskByUnicodeClass(const EUnicodeClass unicodeClass, ui64& mask) {
        switch (unicodeClass) {
            case EUnicodeClass::OTHER: {
                mask |= NthBit64(So_OTHER);
                break;
            }
            case EUnicodeClass::PUNCTUATION: {
                mask |= NthBit64(Pd_DASH) | NthBit64(Pd_HYPHEN) | NthBit64(Ps_START) | NthBit64(Pe_END) |
                        NthBit64(Pc_CONNECTOR) | NthBit64(Po_OTHER) | NthBit64(Po_TERMINAL) | NthBit64(Po_EXTENDER) |
                        NthBit64(Po_HYPHEN) | NthBit64(Po_QUOTE) | NthBit64(Ps_QUOTE) | NthBit64(Pe_QUOTE) |
                        NthBit64(Pi_QUOTE) | NthBit64(Pf_QUOTE);
                break;
            }
            case EUnicodeClass::SINGLE_QUOTE: {
                mask |= NthBit64(Po_SINGLE_QUOTE) | NthBit64(Ps_SINGLE_QUOTE) | NthBit64(Pe_SINGLE_QUOTE) |
                        NthBit64(Pi_SINGLE_QUOTE) | NthBit64(Pf_SINGLE_QUOTE);
                break;
            }
            case EUnicodeClass::CURRENCY: {
                mask |= NthBit64(Sc_CURRENCY);
                break;
            }
            case EUnicodeClass::MATH: {
                mask |= NthBit64(Sm_MATH);
                break;
            }
            case EUnicodeClass::MINUS: {
                mask |= NthBit64(Sm_MINUS);
                break;
            }
            case EUnicodeClass::MODIFIER: {
                mask |= NthBit64(Sk_MODIFIER);
                break;
            }
            case EUnicodeClass::IDEOGRAPH: {
                mask |= NthBit64(Lo_IDEOGRAPH) | NthBit64(Nl_IDEOGRAPH);
                break;
            }
            case EUnicodeClass::KATAKANA: {
                mask |= NthBit64(Lo_KATAKANA);
                break;
            }
            case EUnicodeClass::HIRAGANA: {
                mask |= NthBit64(Lo_HIRAGANA);
                break;
            }
            case EUnicodeClass::HANGUL: {
                mask |= NthBit64(Lo_LEADING) | NthBit64(Lo_VOWEL) | NthBit64(Lo_TRAILING);
                break;
            }
            case EUnicodeClass::CONTROL: {
                mask |= NthBit64(Cf_FORMAT) | NthBit64(Cf_JOIN) | NthBit64(Cf_BIDI) | NthBit64(Cf_ZWNBSP) |
                        NthBit64(Cc_ASCII) | NthBit64(Cc_SPACE) | NthBit64(Cc_SEPARATOR);
                break;
            }
            case EUnicodeClass::COMBINING: {
                mask |= NthBit64(Mc_SPACING) | NthBit64(Mn_NONSPACING) | NthBit64(Me_ENCLOSING);
                break;
            }
            case EUnicodeClass::WHITESPACE: {
                mask |= NthBit64(Cc_SPACE) | NthBit64(Zs_SPACE) | NthBit64(Zs_ZWSPACE) | NthBit64(Zl_LINE) | NthBit64(Zp_PARAGRAPH);
                break;
            }
        }
    }

    TUnicodeProcessingConfig::TUnicodeProcessingConfig(const NProto::TUnicodeProcessingConfig& config)
        : SymbolsToProcess({config.GetSymbolsToProcess().begin(), config.GetSymbolsToProcess().end()})
        , SymbolsToIgnore({config.GetSymbolsToIgnore().begin(), config.GetSymbolsToIgnore().end()})
    {
        for (const TStringBuf unicodeClassToProcess : config.GetClassesToProcess()) {
            SetMaskByUnicodeClass(FromString<EUnicodeClass>(unicodeClassToProcess), ProcessingMask);
        }
    }

    TUnicodeProcessingConfig::TUnicodeProcessingConfig(
        const ui64 processingMask,
        const THashSet<wchar32>& symbolsToProcess,
        const THashSet<wchar32>& symbolsToIgnore)
        : ProcessingMask(processingMask)
        , SymbolsToProcess(symbolsToProcess)
        , SymbolsToIgnore(symbolsToIgnore)
    {
    }

    bool TUnicodeProcessingConfig::IsAllowedSymbol(const wchar32 symbol) const {
        bool toProcess = false;
        bool toIgnore = false;
        if (!SymbolsToProcess.empty())
            toProcess = SymbolsToProcess.contains(symbol);
        if (!SymbolsToIgnore.empty())
            toIgnore = SymbolsToIgnore.contains(symbol);
        return (NUnicode::CharHasType(symbol, ProcessingMask) || toProcess) && !toIgnore;
    }

    bool TUnicodeProcessingConfig::IsAllowedSymbol(const WC_TYPE charType) const {
        return (NthBit64(charType) & ProcessingMask) != 0;
    }

    bool TUnicodeProcessingConfig::IsAllowedSymbol(const NUnicode::NPrivate::TProperty& charProperty) const {
        auto info = charProperty.Info;
        WC_TYPE charType = (WC_TYPE)(info & CCL_MASK);
        return IsAllowedSymbol(charType);
    }

    TConfigurableNormalizer::TConfigurableNormalizer(const NProto::TNormalizationConfig& config)
        : RemoveConfig(config.GetUnicodeRemoveConfig())
        , ReplaceByWhitespaceConfig(config.GetUnicodeReplaceByWhitespaceConfig())
        , SeparateConfig(config.GetUnicodeSeparateConfig())
        , MaxTokenLength(config.GetMaxTokenLength())
        , MaxTokensNumber(config.GetMaxTokensNumber())
        , NeedLowercase(config.GetNeedLowercase())
        , NeedRemoveAttributes(config.GetNeedRemoveAttributes())
        , NeedRemoveTrailingDashStartingTokens(config.GetNeedRemoveTrailingDashStartingTokens())
        , NeedSplitAlphaNumericTokens(config.GetNeedSplitAlphaNumericTokens())
        , NeedKeepTrailingPlusSymbols(config.GetNeedKeepTrailingPlusSymbols())
    {
        if (config.HasUnicodeNormalizationType()) {
            UnicodeNormalizationType = FromString<NUnicode::ENormalization>(config.GetUnicodeNormalizationType());
        }
        NeedKeepTrailingPlusSymbols &= (SeparateConfig.IsAllowedSymbol(plus) |
                ReplaceByWhitespaceConfig.IsAllowedSymbol(plus)); // otherwise this option is useless
    }

    TString TConfigurableNormalizer::Normalize(const TStringBuf text) const {
        return WideToUTF8(
            this->Normalize(
                UTF8ToUTF32<false>(text)));
    }

    TUtf16String TConfigurableNormalizer::Normalize(const TWtringBuf text) const {
        TUtf32String utf32Result = this->Normalize(TUtf32String::FromUtf16(text));
        return UTF32ToWide(utf32Result.data(), utf32Result.length());
    }

    TUtf32String TConfigurableNormalizer::Normalize(const TUtf32StringBuf text) const {
        TUtf32String normalized = UnicodeNormalize(text);
        RemoveExtraSymbols(normalized);
        if (NeedLowercase) {
            ToLower(normalized);
        }
        TList<TUtf32StringBuf> tokens = SplitByWhitespaces(normalized);
        if (NeedRemoveAttributes) {
            RemoveAttributes(tokens);
        }
        if (NeedRemoveTrailingDashStartingTokens) {
            RemoveTrailingDashStartingTokens(tokens);
        }
        ProcessSpecialSymbols(tokens);
        if (NeedSplitAlphaNumericTokens) {
            SplitAlphaNumericTokens(tokens);
        }
        ApplyLimits(tokens);
        return JoinSeq(whitespace, tokens);
    }

    void TBertNormalizerOptimized::GetCharProperties(const TUtf32StringBuf text, TArrayRef<NUnicode::NPrivate::TProperty> charProperties) const {
        for (size_t i = 0; i < text.size(); ++i) {
            charProperties[i] = NUnicode::NPrivate::CharProperty(text[i]);
        }
    }

    void TBertNormalizerOptimized::ToLower(TUtf32String& text, TArrayRef<NUnicode::NPrivate::TProperty> charProperties) const {
        for (size_t i = 0; i < text.size(); ++i) {
            text[i] = static_cast<wchar32>(text[i] + charProperties[i].Lower);
        }
    }

    void TBertNormalizerOptimized::MarkSymbolsProcessingType(TConstArrayRef<NUnicode::NPrivate::TProperty> charProperties, TArrayRef<EProcessingType> charTypes) const {
        for (size_t i = 0; i < charProperties.size(); ++i) {
            if (RemoveConfig.IsAllowedSymbol(charProperties[i])) {
                charTypes[i] = EProcessingType::PT_REMOVE;
            } else if (SeparateConfig.IsAllowedSymbol(charProperties[i])) {
                charTypes[i] = EProcessingType::PT_WRAP;
            } else if (ReplaceByWhitespaceConfig.IsAllowedSymbol(charProperties[i])) {
                charTypes[i] = EProcessingType::PT_TOWHITESPACE;
            }
        }
    }

    void TBertNormalizerOptimized::MarkAttributesToRemove(const TUtf32StringBuf text, TArrayRef<EProcessingType> charTypes) const {
        size_t i = 0;
        size_t tokenStart = 0;
        size_t currentTokenLen = 0;
        while (i < text.size()) {
            switch (charTypes[i]) {
                case EProcessingType::PT_KEEP: {
                    ++currentTokenLen;
                    ++i;
                    break;
                }
                case EProcessingType::PT_REMOVE: {
                    ++i;
                    break;
                }
                case EProcessingType::PT_TOWHITESPACE: {
                    currentTokenLen = 0;
                    tokenStart = i + 1;
                    ++i;
                    break;
                }
                case EProcessingType::PT_WRAP: {
                    if (text[i] == colon) {
                        bool hasSymbolToKeep = false;
                        for (size_t k = 1; i + k < text.size() && charTypes[i + k] != EProcessingType::PT_TOWHITESPACE; ++k) {
                            if (charTypes[i + k] == EProcessingType::PT_KEEP || charTypes[i + k] == EProcessingType::PT_WRAP) {
                                hasSymbolToKeep = true;
                                break;
                            }
                        }
                        if (hasSymbolToKeep) {
                            TUtf16String attribute(Reserve(currentTokenLen));
                            for (size_t k = tokenStart; k < i; ++k) {
                                if (charTypes[k] != EProcessingType::PT_REMOVE) {
                                    WriteSymbol(text[k], attribute);
                                }
                            }
                            if (Singleton<TAttributeSet>()->IsAttribute(attribute)) {
                                for (; tokenStart < i; ++tokenStart) {
                                    charTypes[tokenStart] = EProcessingType::PT_REMOVE;
                                }
                                for (; i < text.size() && charTypes[i] != EProcessingType::PT_TOWHITESPACE; ++i) {
                                    charTypes[i] = EProcessingType::PT_REMOVE;
                                }
                                --i;
                            }
                        }
                    }
                    ++i;
                    break;
                }
                default: {
                    ++i;
                }
            }
        }
    }

    size_t TBertNormalizerOptimized::ApplyLimitsAndCalcOutStringLen(TArrayRef<EProcessingType> charTypes) const {
        size_t i = 0;
        size_t size = 0;
        size_t currentTokenLen = 0;

        while (i < charTypes.size() && charTypes[i] != EProcessingType::PT_WRAP && charTypes[i] != EProcessingType::PT_KEEP) {
            charTypes[i++] = EProcessingType::PT_REMOVE;
        }
        if (i < charTypes.size()) {
            ++size;
            if (charTypes[i] == EProcessingType::PT_WRAP) {
                currentTokenLen = 0;
            } else if (charTypes[i] == EProcessingType::PT_KEEP) {
                ++currentTokenLen;
            }
            ++i;
        }
        while (i < charTypes.size()) {
            switch (charTypes[i]) {
                case EProcessingType::PT_KEEP: {
                    if (currentTokenLen >= MaxTokenLength) {
                        charTypes[i] = EProcessingType::PT_REMOVE;
                    } else {
                        ++size;
                    }
                    if (currentTokenLen == 0) {
                        ++size;
                    }
                    ++currentTokenLen;
                    ++i;
                    break;
                }
                case EProcessingType::PT_TOWHITESPACE: {
                    currentTokenLen = 0;
                    ++i;
                    break;
                }
                case EProcessingType::PT_WRAP: {
                    currentTokenLen = 0;
                    size += 2;
                    ++i;
                    break;
                }
                default: {
                    ++i;
                }
            }
        }
        return (size);
    }

    TUtf32String TBertNormalizerOptimized::FormatOutString(const TUtf32StringBuf text, TArrayRef<EProcessingType> charTypes) const {
        size_t i = 0;
        size_t j = 0;
        bool hasWhitespaceBefore = false;
        bool wasFirstKeepSymbolBefore = false;

        TUtf32String out;
        out.resize(ApplyLimitsAndCalcOutStringLen(charTypes));
        while (i < text.size() && j < out.size()) {
            switch (charTypes[i]) {
                case EProcessingType::PT_KEEP: {
                    if (!wasFirstKeepSymbolBefore)
                        wasFirstKeepSymbolBefore = true;
                    if (hasWhitespaceBefore) {
                        out[j++] = whitespace;
                        out[j++] = text[i++];
                        hasWhitespaceBefore = false;
                    } else {
                        out[j++] = text[i++];
                    }
                    break;
                }
                case EProcessingType::PT_TOWHITESPACE: {
                    hasWhitespaceBefore = true;
                    ++i;
                    break;
                }
                case EProcessingType::PT_WRAP: {
                    if (!wasFirstKeepSymbolBefore) {
                        wasFirstKeepSymbolBefore = true;
                    } else {
                        out[j++] = whitespace;
                    }
                    out[j++] = text[i++];
                    hasWhitespaceBefore = true;
                    break;
                }
                default: {
                    ++i;
                }
            }
        }
        return (out);
    }

    TUtf32String TBertNormalizerOptimized::Normalize(const TUtf32StringBuf text) const {
        TUtf32String normalized = UnicodeNormalize(text);
        TVector<NUnicode::NPrivate::TProperty> charProperties(normalized.size());
        TVector<EProcessingType> charTypes(normalized.size(), EProcessingType::PT_KEEP);

        GetCharProperties(normalized, charProperties);
        if (NeedLowercase) {
            ToLower(normalized, charProperties);
        }
        MarkSymbolsProcessingType(charProperties, charTypes);
        MarkAttributesToRemove(normalized, charTypes);
        return FormatOutString(normalized, charTypes);
    }

    TString TBertNormalizerOptimized::Normalize(const TStringBuf text) const {
        return WideToUTF8(
            this->Normalize(
                UTF8ToUTF32<false>(text)));
    }

    TUtf16String TBertNormalizerOptimized::Normalize(const TWtringBuf text) const {
        TUtf32String utf32Result = this->Normalize(TUtf32String::FromUtf16(text));
        return UTF32ToWide(utf32Result.data(), utf32Result.length());
    }

    NProto::TNormalizationConfig GetCNormConfig() {
        NProto::TNormalizationConfig config;
        config.SetNeedLowercase(true);
        config.SetNeedRemoveAttributes(true);
        config.SetNeedRemoveTrailingDashStartingTokens(true);
        config.SetNeedSplitAlphaNumericTokens(true);
        config.SetNeedKeepTrailingPlusSymbols(true);
        config.SetUnicodeNormalizationType(ToString(NUnicode::ENormalization::NFKC));

        const TVector<wchar32> symbolsToReplaceByWhitespace = {U'+', U'<', U'=', U'>', U'^', U'`', U'|', U'~'};

        NProto::TUnicodeProcessingConfig& replaceByWhitespaceConfig = *(config.MutableUnicodeReplaceByWhitespaceConfig());
        replaceByWhitespaceConfig.AddClassesToProcess(ToString(EUnicodeClass::PUNCTUATION));
        replaceByWhitespaceConfig.AddClassesToProcess(ToString(EUnicodeClass::SINGLE_QUOTE));
        replaceByWhitespaceConfig.AddClassesToProcess(ToString(EUnicodeClass::MINUS));
        for (const wchar32 symbol : symbolsToReplaceByWhitespace) {
            replaceByWhitespaceConfig.AddSymbolsToProcess(symbol);
        }

        NProto::TUnicodeProcessingConfig& separateConfig = *(config.MutableUnicodeSeparateConfig());
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::CURRENCY));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::IDEOGRAPH));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::KATAKANA));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::HIRAGANA));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::HANGUL));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::MATH));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::MODIFIER));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::OTHER));
        for (const wchar32 symbol : symbolsToReplaceByWhitespace) {
            separateConfig.AddSymbolsToIgnore(symbol);
        }

        NProto::TUnicodeProcessingConfig& removeConfig = *(config.MutableUnicodeRemoveConfig());
        removeConfig.AddClassesToProcess(ToString(EUnicodeClass::COMBINING));
        removeConfig.AddClassesToProcess(ToString(EUnicodeClass::CONTROL));

        config.SetMaxTokenLength(32);
        config.SetMaxTokensNumber(20);

        return config;
    }

    NProto::TNormalizationConfig GetBNormConfig() {
        NProto::TNormalizationConfig config;
        config.SetNeedLowercase(true);
        config.SetNeedRemoveAttributes(true);
        config.SetNeedRemoveTrailingDashStartingTokens(false);
        config.SetNeedSplitAlphaNumericTokens(false);
        config.SetNeedKeepTrailingPlusSymbols(false);
        config.SetUnicodeNormalizationType(ToString(NUnicode::ENormalization::NFC));

        NProto::TUnicodeProcessingConfig& separateConfig = *(config.MutableUnicodeSeparateConfig());
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::PUNCTUATION));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::SINGLE_QUOTE));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::CURRENCY));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::IDEOGRAPH));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::MATH));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::MINUS));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::MODIFIER));
        separateConfig.AddClassesToProcess(ToString(EUnicodeClass::OTHER));

        NProto::TUnicodeProcessingConfig& removeConfig = *(config.MutableUnicodeRemoveConfig());
        removeConfig.AddClassesToProcess(ToString(EUnicodeClass::COMBINING));
        removeConfig.AddClassesToProcess(ToString(EUnicodeClass::CONTROL));

        NProto::TUnicodeProcessingConfig& replaceByWhitespaceConfig = *(config.MutableUnicodeReplaceByWhitespaceConfig());
        replaceByWhitespaceConfig.AddClassesToProcess(ToString(EUnicodeClass::WHITESPACE));

        config.SetMaxTokenLength(512);
        config.SetMaxTokensNumber(0);

        return config;
    }

    NProto::TNormalizationConfig GetBNormNFDConfig() {
        NProto::TNormalizationConfig config = GetBNormConfig();
        config.SetUnicodeNormalizationType(ToString(NUnicode::ENormalization::NFD));
        return config;
    }

    NProto::TNormalizationConfig GetANormConfig() {
        NProto::TNormalizationConfig config;
        config.SetNeedLowercase(false);
        config.SetNeedRemoveAttributes(true);
        config.SetNeedRemoveTrailingDashStartingTokens(false);
        config.SetNeedSplitAlphaNumericTokens(false);
        config.SetNeedKeepTrailingPlusSymbols(false);
        config.ClearUnicodeNormalizationType();
        config.SetMaxTokenLength(0);
        config.SetMaxTokensNumber(0);
        return config;
    }

    TConsistentNormalizer::TConsistentNormalizer()
        : TConfigurableNormalizer(GetCNormConfig())
    {
    }

    TBertNFDNormalizer::TBertNFDNormalizer()
        : TConfigurableNormalizer(GetBNormNFDConfig())
    {
    }

    TBertNormalizer::TBertNormalizer()
        : TConfigurableNormalizer(GetBNormConfig())
    {
    }

    TBertNormalizerOptimized::TBertNormalizerOptimized()
        : TConfigurableNormalizer(GetBNormConfig())
    {
    }

    TAttributeNormalizer::TAttributeNormalizer()
        : TConfigurableNormalizer(GetANormConfig())
    {
    }

} //NCnorm

TString NCnorm::Cnorm(const TStringBuf in) {
    return Singleton<TConsistentNormalizer>()->Normalize(in);
}

TUtf16String NCnorm::Cnorm(const TWtringBuf in) {
    return Singleton<TConsistentNormalizer>()->Normalize(in);
}

TUtf32String NCnorm::Cnorm(const TUtf32StringBuf in) {
    return Singleton<TConsistentNormalizer>()->Normalize(in);
}

TString NCnorm::BNormOld(const TStringBuf in) {
    return Singleton<TBertNormalizer>()->Normalize(in);
}

TString NCnorm::BNorm(const TStringBuf in) {
    return Singleton<TBertNormalizerOptimized>()->Normalize(in);
}

TUtf16String NCnorm::BNorm(const TWtringBuf in) {
    return Singleton<TBertNormalizerOptimized>()->Normalize(in);
}

TUtf32String NCnorm::BNorm(const TUtf32StringBuf in) {
    return Singleton<TBertNormalizerOptimized>()->Normalize(in);
}

TString NCnorm::BNormNFD(const TStringBuf in) {
    return Singleton<TBertNFDNormalizer>()->Normalize(in);
}

TUtf16String NCnorm::BNormOld(const TWtringBuf in) {
    return Singleton<TBertNormalizer>()->Normalize(in);
}

TUtf16String NCnorm::BNormNFD(const TWtringBuf in) {
    return Singleton<TBertNFDNormalizer>()->Normalize(in);
}

TUtf32String NCnorm::BNormOld(const TUtf32StringBuf in) {
    return Singleton<TBertNormalizer>()->Normalize(in);
}

TUtf32String NCnorm::BNormNFD(const TUtf32StringBuf in) {
    return Singleton<TBertNFDNormalizer>()->Normalize(in);
}

TString NCnorm::ANorm(const TStringBuf in) {
    return Singleton<TAttributeNormalizer>()->Normalize(in);
}

TUtf16String NCnorm::ANorm(const TWtringBuf in) {
    return Singleton<TAttributeNormalizer>()->Normalize(in);
}

TUtf32String NCnorm::ANorm(const TUtf32StringBuf in) {
    return Singleton<TAttributeNormalizer>()->Normalize(in);
}
