#pragma once

#include <library/cpp/langmask/langmask.h>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>

/*
 * Parses config of the following format:
 * <Language Mask="comma-separated list of languages">
 *    Param1=Value1
 *    Param2=Value2
 *    ...
 * </Language>
 * ...
 * <Language Mask="comma-separated list of languages">
 *    Param1=Value1
 *    Param2=Value2
 *    ...
 * </Language>
 */

namespace NLangConfig {

struct TLangSection {
    TLangMask Langs;
    THashMap<TCiString, TString> Params;
};

// Can throw yexception in case of any config errors
// requiredParams - comma-separated list of required section params
TVector<TLangSection> ParseConfig(const TString& cfgPath, const TStringBuf& requiredParams = TStringBuf());

} // NLangConfig
