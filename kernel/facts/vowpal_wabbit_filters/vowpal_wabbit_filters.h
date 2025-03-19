#pragma once

#include <kernel/facts/vowpal_wabbit_filters/vowpal_wabbit_config.sc.h>

#include <library/cpp/vowpalwabbit/vowpal_wabbit_model.h>
#include <library/cpp/vowpalwabbit/vowpal_wabbit_predictor.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/domscheme_traits.h>

#include <util/generic/maybe.h>
#include <util/string/builder.h>
#include <util/folder/path.h>
#include <util/stream/file.h>


namespace NVwFilters {

static constexpr TStringBuf NS_QUERY = "Q";
static constexpr TStringBuf NS_ANSWER = "A";


class TConfigValidationHelper {
public:
    explicit TConfigValidationHelper(size_t maxErrors = 3);

    TString ValidateNamespaces(const NDomSchemeRuntime::TConstArray<TSchemeTraits, NDomSchemeRuntime::TConstPrimitive<TSchemeTraits, TStringBuf>>& namespaces) const;
    void operator()(TStringBuf key, TStringBuf errmsg, NDomSchemeRuntime::TValidateInfo vi);
    TString ToString() const;

private:
    TStringBuilder ErrorMessage;
    const size_t MaxErrors;
    size_t Errors;
};


class TQueryData {
public:
    TQueryData(const TString& query, const TString& answer);

    const TString& GetQuery() const;
    const TString& GetAnswer() const;

    const TVector<TString>& GetQueryTokens() const;
    const TVector<TString>& GetAnswerTokens() const;

    static TVector<TString> Tokenize(const TString& text);

private:
    const TString Query;
    const TString Answer;
    mutable TMaybe<TVector<TString>> QueryTokens;
    mutable TMaybe<TVector<TString>> AnswerTokens;
};


struct TFilterResult {
    TFilterResult()
        : Filtered(false)
        , FilteredBy("")
        , Error("")
    {
    }
    TFilterResult(const bool filtered, const TString& filteredBy, const TString& error)
        : Filtered(filtered)
        , FilteredBy(filteredBy)
        , Error(error)
    {
    }

    bool Filtered;
    TString FilteredBy;
    TString Error;
};


class TVowpalWabbitMultiFilter {
public:
    TVowpalWabbitMultiFilter(const TFsPath& fmlsDir, const TFsPath& configPath);

    TFilterResult Filter(const TString& query, const TString& answer, const NSc::TValue& configPatch) const;

private:
    static bool HasNamespace(const TVwFilterConfigConst<TSchemeTraits>& config, const TStringBuf& ns);

private:
    NSc::TValue Config;
    THashMap<TString, TVowpalWabbitModel> Models;
    THashMap<TString, TVowpalWabbitPredictor> Predictors;
};

} // namespace NVwFilters
