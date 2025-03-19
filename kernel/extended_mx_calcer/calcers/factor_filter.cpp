#include "bundle.h"
#include "random.h"
#include <library/cpp/expression/expression.h>

using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFactorFilterBundle - wrapper for calcers with filter by factor values
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const TString FACTORS_PREFIX = "f.";

class TFactorFilterBundle : public TBundleBase<TFactorFilterConstProto> {
    TVector<TString> FactorTokens;
    THashMap<TString, size_t> TokenToNumber;
    size_t MaxFactorNumber = 0;

public:
    TFactorFilterBundle(const NSc::TValue& scheme)
        : TBundleBase(scheme)
    {
        GetMeta().RegisterAttr(NMeta::RANDOM_SEED);
        TExpression(TString{*Scheme().Params().Expression()}).GetTokensWithPrefix(FACTORS_PREFIX, FactorTokens);
        for (const auto& token : FactorTokens) {
            size_t factorNumber = FromString(TStringBuf(token).substr(FACTORS_PREFIX.size()));
            TokenToNumber[token] = factorNumber;
            MaxFactorNumber = Max(MaxFactorNumber, factorNumber + 1);
        }
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "Factor filter bundle calculation started\n";
        THashMap<TString, TString> h;
        const auto& params = Scheme().Params();
        const TString seed = TString::Join(context.GetMeta().RandomSeed().Get(), params.RandomAdditionalSeed());
        context.DbgLog() << "seed = " << seed << '\n';
        auto setAndPrint = [&](const TString& k, const TString& v) {
            h[k] = v;
            context.DbgLog() << k << " = " << v << '\n';
        };
        NExtendedMx::TRandom random(seed);
        setAndPrint("rand1", ToString(random.NextReal(1.f)));
        for (const auto& FactorTokenNumberPair : TokenToNumber) {
            setAndPrint(FactorTokenNumberPair.first, ToString(features[FactorTokenNumberPair.second]));
        }
        double res = CalcExpression(TString{*Scheme().Params().Expression()}, h);
        res = ApplyResultTransform(res, Scheme().ResultTransform());
        LogValue(context, "result", res);
        return res;
    }

    size_t GetNumFeats() const override {
        return MaxFactorNumber;
    }

};


TExtendedCalculatorRegistrator<TFactorFilterBundle> FactorFilterBundleRegistrator("factor_filter");
