#pragma once

#include <kernel/search_query/proto/config.pb.h>

#include <library/cpp/unicode/normalization/normalization.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/array_ref.h>
#include <util/ysaveload.h>


namespace NCnorm { // Configurable normalization; TODO: rename to NCNorm to follow slyleguide

    enum class EUnicodeClass {
        PUNCTUATION,
        SINGLE_QUOTE,
        CURRENCY,
        MATH,
        MINUS,
        MODIFIER,
        IDEOGRAPH,
        KATAKANA,
        HIRAGANA,
        HANGUL,
        OTHER,
        CONTROL,
        COMBINING,
        WHITESPACE,
    };

    enum EProcessingType {
        PT_REMOVE,
        PT_KEEP,
        PT_WRAP,
        PT_TOWHITESPACE,
    }; 

    class TUnicodeProcessingConfig {
    public:
        TUnicodeProcessingConfig() = default;
        TUnicodeProcessingConfig(const NProto::TUnicodeProcessingConfig& config);
        TUnicodeProcessingConfig(const ui64 processingMask, const THashSet<wchar32>& symbolsToProcess,
            const THashSet<wchar32>& symbolsToIgnore);

        bool IsAllowedSymbol(const wchar32 symbol) const;
        bool IsAllowedSymbol(const WC_TYPE charType) const;
        bool IsAllowedSymbol(const NUnicode::NPrivate::TProperty& charProperty) const;

    public:
        Y_SAVELOAD_DEFINE(
            ProcessingMask,
            SymbolsToProcess,
            SymbolsToIgnore
        );

    private:
        ui64 ProcessingMask = 0;
        THashSet<wchar32> SymbolsToProcess;
        THashSet<wchar32> SymbolsToIgnore;
    };

    class TConfigurableNormalizer {
    public:
        TConfigurableNormalizer() = delete;
        TConfigurableNormalizer(const NProto::TNormalizationConfig& config);

        TString Normalize(const TStringBuf text) const;
        TUtf16String Normalize(const TWtringBuf text) const;
        TUtf32String Normalize(const TUtf32StringBuf text) const;

    public:
        Y_SAVELOAD_DEFINE(
            RemoveConfig,
            ReplaceByWhitespaceConfig,
            SeparateConfig,
            MaxTokenLength,
            MaxTokensNumber,
            UnicodeNormalizationType,
            NeedLowercase,
            NeedRemoveAttributes,
            NeedRemoveTrailingDashStartingTokens,
            NeedSplitAlphaNumericTokens,
            NeedKeepTrailingPlusSymbols
        );

    protected:
        TUtf32String UnicodeNormalize(const TUtf32StringBuf text) const;
        void RemoveExtraSymbols(TUtf32String& text) const;
        TList<TUtf32StringBuf> SplitByWhitespaces(const TUtf32StringBuf text) const;
        void RemoveAttributes(TList<TUtf32StringBuf>& tokens) const;
        void RemoveTrailingDashStartingTokens(TList<TUtf32StringBuf>& tokens) const;
        void ProcessSpecialSymbols(TList<TUtf32StringBuf>& tokens) const;
        void SplitAlphaNumericTokens(TList<TUtf32StringBuf>& tokens) const;
        void ApplyLimits(TList<TUtf32StringBuf>& tokens) const;

    protected: 
        TUnicodeProcessingConfig RemoveConfig;
        TUnicodeProcessingConfig ReplaceByWhitespaceConfig;
        TUnicodeProcessingConfig SeparateConfig;

        ui64 MaxTokenLength;
        ui64 MaxTokensNumber;

        TMaybe<NUnicode::ENormalization> UnicodeNormalizationType;

        bool NeedLowercase;
        bool NeedRemoveAttributes;
        bool NeedRemoveTrailingDashStartingTokens;
        bool NeedSplitAlphaNumericTokens;
        bool NeedKeepTrailingPlusSymbols;
    };

    NProto::TNormalizationConfig GetCNormConfig();
    NProto::TNormalizationConfig GetBNormNFDConfig();
    NProto::TNormalizationConfig GetBNormConfig();
    NProto::TNormalizationConfig GetANormConfig();

    class TConsistentNormalizer : public TConfigurableNormalizer {
    public:
        TConsistentNormalizer();
        TConsistentNormalizer(const NProto::TNormalizationConfig& config) = delete;
    };

    class TBertNFDNormalizer : public TConfigurableNormalizer {
    public:
        TBertNFDNormalizer();
        TBertNFDNormalizer(const NProto::TNormalizationConfig& config) = delete;
    };

    class TBertNormalizer : public TConfigurableNormalizer {
    public:
        TBertNormalizer();
        TBertNormalizer(const NProto::TNormalizationConfig& config) = delete;
    };

    class TBertNormalizerOptimized : public TConfigurableNormalizer {
    public:
        TBertNormalizerOptimized();
        TBertNormalizerOptimized(const NProto::TNormalizationConfig& config) = delete;
    
        TString Normalize(const TStringBuf text) const;
        TUtf16String Normalize(const TWtringBuf text) const;
        TUtf32String Normalize(const TUtf32StringBuf text) const;

    private:
        void GetCharProperties(const TUtf32StringBuf text, TArrayRef<NUnicode::NPrivate::TProperty> charProperties) const;
        void MarkSymbolsProcessingType(TConstArrayRef<NUnicode::NPrivate::TProperty> charProperties, TArrayRef<EProcessingType> charTypes) const;
        void MarkAttributesToRemove(const TUtf32StringBuf text, TArrayRef<EProcessingType> charTypes) const;

    private:
        void ToLower(TUtf32String& text, TArrayRef<NUnicode::NPrivate::TProperty> charProperties) const;
        TUtf32String FormatOutString(const TUtf32StringBuf text, TArrayRef<EProcessingType> charTypes) const;
        size_t ApplyLimitsAndCalcOutStringLen(TArrayRef<EProcessingType> charTypes) const;
    };

    class TAttributeNormalizer : public TConfigurableNormalizer {
    public:
        TAttributeNormalizer();
        TAttributeNormalizer(const NProto::TNormalizationConfig& config) = delete;
    };

    TString Cnorm(const TStringBuf);  // TODO: rename to CNorm to follow slyleguide
    TUtf16String Cnorm(const TWtringBuf);
    TUtf32String Cnorm(const TUtf32StringBuf);

    TString BNormNFD(const TStringBuf);
    TUtf16String BNormNFD(const TWtringBuf);
    TUtf32String BNormNFD(const TUtf32StringBuf);

    TString BNormOld(const TStringBuf);
    TUtf16String BNormOld(const TWtringBuf);
    TUtf32String BNormOld(const TUtf32StringBuf);

    TString BNorm(const TStringBuf);
    TUtf16String BNorm(const TWtringBuf);
    TUtf32String BNorm(const TUtf32StringBuf);

    TString ANorm(const TStringBuf);
    TUtf16String ANorm(const TWtringBuf);
    TUtf32String ANorm(const TUtf32StringBuf);

} // NCnorm
