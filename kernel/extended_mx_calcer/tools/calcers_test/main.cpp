#include <kernel/extended_mx_calcer/factory/factory.h>

#include <library/cpp/json/json_prettifier.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/file.h>
#include <util/string/split.h>
#include <library/cpp/streams/factory/factory.h>

using TFactors = TVector<float>;

using namespace NExtendedMx;

template <typename T>
static TVector<T> Split(const TString& s, char delim, bool skipEmpty = false) {
    TVector<T> res;
    for (const auto& part : StringSplitter(s).Split(delim)) {
        if (skipEmpty && !part.Token())
            continue;
        res.push_back(FromString<T>(part.Token()));
    }
    return res;
}

NSc::TValue DoTest(const TString& fmlPath, const TFactors& factors, const NSc::TValue& ctx, const TString& recalcParams, const TString& AdditionalSeed) {
    NSc::TValue res;

    try {
        TExtendedLoaderFactory factory;
        const auto& calcer = factory.Load(fmlPath);
        calcer->Initialize();
        res["num_feats"] = calcer->GetNumFeats();

        for (const auto& kv : calcer->ViewMeta()) {
            res["meta_attrs"][kv.first] = kv.second;
        }
        if (auto* info = calcer->GetInfo()) {
            for (const auto& kv : *info) {
                res["info_attrs"][kv.first] = kv.second;
            }
        } else {
            res["info_attrs"] = "GetInfo returns nullptr";
        }
        TCalcContext calcCtx(ctx.Clone());
        calcCtx.GetMeta().RandomSeed() = calcCtx.GetMeta().RandomSeed() + AdditionalSeed;
        double val = 0.;
        if (calcCtx.GetMeta().HasCalcerPatch()) {
            const NSc::TValue patch = calcCtx.GetMeta().CalcerPatch().GetRawValue()->Get(calcer->GetAlias());
            const auto cloned = calcer->CloneAndOverrideParams(patch, calcCtx);
            val = cloned->CalcRelevExtended(factors, calcCtx);
            res["cloned"] = 1;
        } else {
            val = calcer->CalcRelevExtended(factors, calcCtx);
        }
        res["value"] = val;
        if (!recalcParams.empty()) {
            const NSc::TValue recalcParamsParsed = NSc::TValue::FromJsonThrow(recalcParams);
            val = calcer->RecalcRelevExtended(calcCtx, recalcParamsParsed);
            res["recalc_value"] = val;
        }
        res["context"] = calcCtx.Root();
        res["debug"] = calcCtx.DbgLog().ToString();
    } catch (...) {
        res["exception"] = CurrentExceptionMessage();
    }
    return res;
}

void Test(const TString& fmlPath, const TString& factorsSetPath, const TString& jsonCtx, const TString& recalcParams) {
    TString factorsSet = OpenInput(factorsSetPath)->ReadAll();
    const auto ctx = NSc::TValue::FromJsonThrow(jsonCtx);
    size_t fSetId = 1;
    for (const auto& factorsLine : Split<TString>(factorsSet, '\n')) {
        const auto& factors = Split<float>(factorsLine, '\t', true);
        auto res = DoTest(fmlPath, factors, ctx, recalcParams, ToString(fSetId));
        Cout << "factor set: " << fSetId++ << "\tresult: " << NJson::PrettifyJson(res.ToJson(), true, 4, true) << Endl;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Simple tests calcers
// expected stdin:
//   path_to_formula \t path_to_factors.tsv \t input json ctx
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    using namespace NLastGetopt;
    TOpts opts = TOpts::Default();

    TString fmlPath;
    TString factorSetPath;
    TString jsonCtx;
    TString recalcParams;
    opts.AddCharOption('f', "Formula").Required().StoreResult(&fmlPath);
    opts.AddCharOption('s', "FactorSet").Required().StoreResult(&factorSetPath);
    opts.AddCharOption('c', "Ctx").Required().StoreResult(&jsonCtx);
    opts.AddCharOption('r', "RecalcParams").Optional().StoreResult(&recalcParams, "");

    TOptsParseResult parseResult(&opts, argc, argv);
    Test(fmlPath, factorSetPath, jsonCtx, recalcParams);
    return 0;
}
