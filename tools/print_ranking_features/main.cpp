#include "main.h"

#include <kernel/ranking_feature_pool/ranking_feature_pool.h>
#include <kernel/factor_slices/factor_domain.h>
#include <kernel/feature_pool/feature_filter/feature_filter.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_prettifier.h>
#include <library/cpp/streams/factory/factory.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/string/util.h>

#include <google/protobuf/messagext.h>
#include <google/protobuf/text_format.h>

class TFeatureExpression {
public:
    explicit TFeatureExpression(const TString& expr) {
        DoParse(expr);
    }

    bool operator()(const NMLPool::TFeatureInfo& info) const {
        return (!FeatureFilter || FeatureFilter->Check(info));
    }

private:
    void DoParse(const TString& expr) {
        static const char nameChar = '/';
        static const char tagsChar = ':';

        if (!expr) {
            return;
        }

        TStringBuf buf = expr;

        size_t namePos = buf.find(nameChar);
        size_t tagsPos = buf.find(tagsChar);

        if (namePos != TStringBuf::npos && tagsPos != TStringBuf::npos) {
            Y_ENSURE(namePos < tagsPos, "syntax error: char '" << tagsChar
                << "' occurs before '" << nameChar << "'");
        }

        TString tagsExpr;
        if (tagsPos != TStringBuf::npos) {
            TStringBuf tok;
            buf.RNextTok(tagsChar, tok);
            tagsExpr = TString(tok);
        }

        TString nameExpr;
        if (namePos != TStringBuf::npos) {
            TStringBuf tok;
            buf.RNextTok(nameChar, tok);
            nameExpr = TString(tok);
        }

        TString sliceExpr = TString(buf);

        if (!!sliceExpr || !!nameExpr || !!tagsExpr) {
            FeatureFilter.Reset(new NMLPool::TFeatureFilter());
            FeatureFilter->SetSliceExpr(sliceExpr);
            FeatureFilter->SetNameExpr(nameExpr);
            FeatureFilter->SetTagExpr(tagsExpr);
        }
    }

private:
    THolder<NMLPool::TFeatureFilter> FeatureFilter;
};

void PrepareGlobalPoolInfo(NMLPool::TPoolInfo& poolInfo) {
    NFactorSlices::TSlicesMetaInfo metaInfo = NFactorSlices::TGlobalSlicesMetaInfo::Instance();
    for (NFactorSlices::TLeafIterator iter; iter.Valid(); iter.Next()) {
        if (metaInfo.IsSliceInitialized(*iter)) {
            NFactorSlices::EnableSlices(metaInfo, *iter);
        }
    }
    NFactorSlices::TFactorDomain domain(metaInfo);
    NRankingFeaturePool::MakePoolInfo(domain, poolInfo);
}

TVector<size_t> CollectFeatureIndices(const NMLPool::TPoolInfo& poolInfo,
    const TVector<TFeatureExpression>& exprs)
{
    TVector<size_t> res;
    size_t index = 0;
    for (const auto& info : poolInfo.GetFeatureInfo()) {
        if (exprs.empty()) {
            res.push_back(index);
        } else {
            for (const auto& expr : exprs) {
                if (expr(info)) {
                    res.push_back(index);
                    break;
                }
            }
        }
        index += 1;
    }
    return res;
}

class TPrintFeaturesContext {
public:
    struct TOptions {
        TVector<TString> FeatureExprs;
        EOutputFormat OutputFormat{EOutputFormat::Proto};
    };

public:
    explicit TPrintFeaturesContext(const TOptions& options)
        : Options(options)
    {
        PrepareGlobalPoolInfo(PoolInfo);

        for (const auto& exprStr : Options.FeatureExprs) {
            try {
                Exprs.emplace_back(exprStr);
            } catch (yexception&) {
                ythrow yexception() << "failed to parse feature expression: \"" << exprStr << "\""
                    << "\n<parser message>: " << CurrentExceptionMessage();
            }
        }

        Indices = CollectFeatureIndices(PoolInfo, Exprs);
        for (size_t index : Indices) {
            auto& featureInfo = *FilteredPoolInfo.AddFeatureInfo();
            featureInfo.CopyFrom(PoolInfo.GetFeatureInfo(index));
            if (EOutputFormat::Proto == Options.OutputFormat) {
                featureInfo.ClearExtJson();
            }
        }
    }

    void PrintFeatures(IOutputStream& out) const {
        switch (Options.OutputFormat) {
            case EOutputFormat::Proto: {
                TString text;
                ::google::protobuf::TextFormat::PrintToString(FilteredPoolInfo, &text);
                out << text << Endl;
                break;
            }
            case EOutputFormat::Json:
            case EOutputFormat::ExtJson: {
                const bool extInfo = (EOutputFormat::ExtJson == Options.OutputFormat);
                NJson::PrettifyJson(ToString(NMLPool::ToJson(FilteredPoolInfo, extInfo)), out);
                out << Endl;
                break;
            }
        }
    }

private:
    TOptions Options;
    NMLPool::TPoolInfo PoolInfo;
    NMLPool::TPoolInfo FilteredPoolInfo;
    TVector<TFeatureExpression> Exprs;
    TVector<size_t> Indices;
};

int main(int argc, const char* argv[]) {
    TPrintFeaturesContext::TOptions printOptions;
    NLastGetopt::TOpts options;

    options.SetTitle("Print information about features in Arcadia."
        "\nTo be accessible, features should be registered in one of slices."
        "\nLibrary that describes the slice should be linked with this executable.");

    options.AddHelpOption('h');

    options.AddLongOption('f', "format", "Output format")
        .RequiredArgument("STR")
        .DefaultValue("proto")
        .StoreResult(&printOptions.OutputFormat);

    options.SetFreeArgDefaultTitle("<feature-expr>", "Feature expression: [<slice-re>]['/'<name-re>][':'<tag-expr>]");

    NLastGetopt::TOptsParseResult optionsResult(&options, argc, argv);

    printOptions.FeatureExprs = optionsResult.GetFreeArgs();
    TPrintFeaturesContext printer(printOptions);
    printer.PrintFeatures(Cout);
}
