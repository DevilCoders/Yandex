#include "vowpal_wabbit_filters.h"

#include <util/string/subst.h>
#include <util/folder/iterator.h>


namespace NVwFilters {

// ----- TConfigValidationHelper -----

TConfigValidationHelper::TConfigValidationHelper(size_t maxErrors)
    : MaxErrors(maxErrors)
    , Errors(0)
{
}

TString TConfigValidationHelper::ValidateNamespaces(const NDomSchemeRuntime::TConstArray<TSchemeTraits, NDomSchemeRuntime::TConstPrimitive<TSchemeTraits, TStringBuf>>& namespaces) const {
    for (const TStringBuf ns : namespaces) {
        if (ns != NS_QUERY && ns != NS_ANSWER) {
            return "unknown namespace " + ::ToString(ns);
        }
    }
    return "";
}

void TConfigValidationHelper::operator()(TStringBuf key, TStringBuf errmsg, NDomSchemeRuntime::TValidateInfo vi) {
    if (vi.Severity == NDomSchemeRuntime::TValidateInfo::ESeverity::Warning) {
        return;
    }
    if (++Errors <= MaxErrors) {
        ErrorMessage << key << ": " << errmsg << "; ";
    }
}

TString TConfigValidationHelper::ToString() const {
    if (Errors > MaxErrors) {
        return ErrorMessage + ::ToString(Errors - MaxErrors) + " more errors";
    }
    return ErrorMessage;
}


// ----- TQueryData -----

TQueryData::TQueryData(const TString& query, const TString& answer)
    : Query(query)
    , Answer(answer)
{
}

const TString& TQueryData::GetQuery() const {
    return Query;
}

const TString& TQueryData::GetAnswer() const {
    return Answer;
}

const TVector<TString>& TQueryData::GetQueryTokens() const {
    if (!QueryTokens) {
        QueryTokens = Tokenize(Query);
    }
    return *QueryTokens;
}

const TVector<TString>& TQueryData::GetAnswerTokens() const {
    if (!AnswerTokens) {
        AnswerTokens = Tokenize(Answer);
    }
    return *AnswerTokens;
}

TVector<TString> TQueryData::Tokenize(const TString& text) {
    TVector<TString> tokens = SplitString(text, " ");
    for (TString& token : tokens) {
        SubstGlobal(token, "|", "");
        SubstGlobal(token, ":", "");
    }
    return tokens;
}


// ----- TVowpalWabbitMultiFilter -----

bool TVowpalWabbitMultiFilter::HasNamespace(const TVwFilterConfigConst<TSchemeTraits>& config, const TStringBuf& ns) {
    return std::find(config.Namespaces().begin(), config.Namespaces().end(), ns) != config.Namespaces().end();
}

TVowpalWabbitMultiFilter::TVowpalWabbitMultiFilter(const TFsPath& fmlsDir, const TFsPath& configPath) {
    TFileInput configFile(configPath);
    Config = NSc::TValue::FromJson(configFile.ReadAll());

    auto iterator = TDirIterator(fmlsDir);
    while (auto entry = iterator.Next()) {
        TFsPath path = entry->fts_path;
        if (!path.IsFile()) {
            continue;
        }
        if (path.GetExtension() != "vw") {
            continue;
        }
        const TString filterName = path.GetName();
        Models.emplace(filterName, path);
        Predictors.emplace(filterName, Models.at(filterName));
    }
}

TFilterResult TVowpalWabbitMultiFilter::Filter(const TString& query, const TString& answer, const NSc::TValue& configPatch) const {
    TQueryData queryData(query, answer);

    NSc::TValue rawConfig;
    if (!configPatch.IsNull()) {
        rawConfig.CopyFrom(Config).MergeUpdate(configPatch);
    } else {
        rawConfig = Config;
    }
    TVwMultiFilterConfigConst<TSchemeTraits> config(&rawConfig);
    TConfigValidationHelper helper;
    if (!config.Validate("", false, std::ref(helper), &helper)) {
        return {false, "", "Config validation error: " + helper.ToString()};
    }

    for (const auto& kv : config.Filters()) {
        const auto& filterName = kv.Key();
        const auto& filterConfig = kv.Value();

        if (!filterConfig.Enabled()) {
            continue;
        }

        if (!Predictors.contains(filterConfig.Fml())) {
            return {false, "", "Model " + ToString(filterConfig.Fml()) + " not found"};
        }
        const TVowpalWabbitPredictor& predictor = Predictors.at(filterConfig.Fml());

        float prediction = predictor.GetConstPrediction();
        if (HasNamespace(filterConfig, NS_QUERY)) {
            prediction += predictor.Predict(NS_QUERY, queryData.GetQueryTokens(), filterConfig.Ngram());
        }
        if (HasNamespace(filterConfig, NS_ANSWER)) {
            prediction += predictor.Predict(NS_ANSWER, queryData.GetAnswerTokens(), filterConfig.Ngram());
        }
        if (prediction >= filterConfig.Threshold()) {
            return {true, ToString(filterName), ""};
        }
    }

    return {false, "", ""};
}

} // namespace NVwFilters
