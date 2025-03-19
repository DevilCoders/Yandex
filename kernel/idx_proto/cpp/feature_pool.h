#pragma once

#include <util/generic/yexception.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>

#include <kernel/idx_proto/feature_pool.pb.h>

inline TString ESearchTypeToString(const NFeaturePool::ESearchType searchType) {
    switch (searchType) {
        case NFeaturePool::ST_DEFAULT: return "DEFAULT";
        case NFeaturePool::ST_EXTENDED: return "EXTENDED";
        case NFeaturePool::ST_NOT_FOUND: return "NOT_FOUND";
        case NFeaturePool::ST_TRASH: return "TRASH";
    }
    throw yexception() << "Unknown search type " << int(searchType) <<
        " got from NFeaturePool::ESearchType protobuf structure item. ";
}

inline NFeaturePool::ESearchType ESearchTypeFromString(const TStringBuf& strSearchType) {
    // more ofter used fields are checker earlier
    if (strSearchType == TStringBuf("DEFAULT"))
        return NFeaturePool::ST_DEFAULT;
    else if (strSearchType == TStringBuf("EXTENDED"))
        return NFeaturePool::ST_EXTENDED;
    else if (strSearchType == TStringBuf("NOT_FOUND"))
        return NFeaturePool::ST_NOT_FOUND;

    throw yexception() << "Unknown search type: " << TString{strSearchType}.Quote() << ". ";
}

inline TString ENormalizationTypeToString(const NFeaturePool::ENormalizationType normType) {
    switch (normType) {
        case NFeaturePool::NT_UNKNOWN: ythrow yexception() << "This type of normalization should not be converted to string. ";
        case NFeaturePool::NT_WEAK: return "weak";
        case NFeaturePool::NT_STRONG: return "strong";
        case NFeaturePool::NT_OLD: return "old";
    }
    throw yexception() << "Unknown normalization type " << int(normType) <<
        " got from NFeaturePool::ENormalizationType protobuf structure item. ";
}

inline TString EFiltrationTypeToString(const NFeaturePool::EFiltrationType filtrationType) {
    switch (filtrationType) {
        case NFeaturePool::FT_DUPLICATE: return "DUPLICATE";
        case NFeaturePool::FT_NOT_FILTERED: return "NOT_FILTERED";
        case NFeaturePool::FT_NOT_FOUND_SKIPPED: return "NOT_FOUND";
        case NFeaturePool::FT_SAME_ROBOT_DIFFERENT_RANKING: return "SAME_ROBOT_DIFFERENT_RANKING";
        case NFeaturePool::FT_STRONG_WITH_WEAK: return "STRONG_WITH_WEAK";
        case NFeaturePool::FT_TIER_PRIORITY: return "TIER_PRIORITY";
        case NFeaturePool::FT_TIER_WEAK_EXISTS: return "TIER_WEAK_EXISTS";
        case NFeaturePool::FT_TOO_MANY_STRONG: return "TOO_MANY_STRONG";
        case NFeaturePool::FT_MULTILANG_PRIORITY: return "FT_MULTILANG_PRIORITY";
        case NFeaturePool::FT_UNKNOWN: return "FT_UNKNOWN";
    }
    throw yexception() << "Unknown filtration type: " << int(filtrationType) <<
        " got from NFeaturePool::EFiltrationType protobuf structure item. ";
}

inline TString NormalizationMaskToString(ui32 normMask) {
    // spike for testing to comply with older versions
    if ((normMask & NFeaturePool::NT_WEAK) &&
        (normMask & NFeaturePool::NT_STRONG) &&
        !(normMask & NFeaturePool::NT_OLD))
        return TString("all");

    TString result;
    if (normMask & NFeaturePool::NT_WEAK)
        result += "|weak";
    if (normMask & NFeaturePool::NT_STRONG)
        result += "|strong";
    if (normMask & NFeaturePool::NT_OLD)
        result += "|old";
    if (result.size())
        return result.substr(1);
    return result;
}

inline ui32 NormalizationMaskFromString(const TStringBuf& strNormMask) {
    ui32 normMask = 0;
    if (strNormMask == TStringBuf("all"))
        normMask = NFeaturePool::NT_WEAK | NFeaturePool::NT_STRONG;
    else if (strNormMask == TStringBuf("weak"))
        normMask = NFeaturePool::NT_WEAK;
    else if (strNormMask == TStringBuf("strong"))
        normMask = NFeaturePool::NT_STRONG;
    else
        throw yexception() << "Unknown normalization mask " << TString{strNormMask}.Quote() << ". ";

    return normMask;
}

/**
 * Appends @c count features with @c value value (for Zeus needs)
 **/
inline void AppendDefaultFeatureValues(NFeaturePool::TLine& poolLine, size_t count, const float value) {
    for (size_t i = 0; i < count; ++i) {
        poolLine.AddFeature(value);
    }
}

/**
 * Compatibility function for converting features.tsv to pool lines
 * https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/PodborFormul/kosher-pool-file-format
 * You should NOT use this function, use protopool TLine directly.
 * @param featuresLine tab-separated line of features.tsv file
 * @param line output proto-line
 **/
void FeaturesTSVLineToProto(const TStringBuf& featuresLine, NFeaturePool::TLine& line);

/**
 * Opposite function to FeaturesTSVLineToProto
 * You should NOT use this function, use protopool TLine directly.
 * @param line input proto-line
 * @param featuresLine tab-separated line for features.tsv file
 * @param addRequestdId if disabled, will skip first tab-column in result
**/
void ProtoToFeaturesTSVLine(const NFeaturePool::TLine& line, TString& featuresLine, bool addRequestdId = true);

/**
 * Determines was pool line filtered or not
 * @param line pool proto-line
**/
inline bool IsPoolLineFiltered(const NFeaturePool::TLine& line) {
    // if filtration type is unset, it defaults to FT_UNKNOWN and will be filtered
    return line.GetFiltrationType() != NFeaturePool::FT_NOT_FILTERED;
}
