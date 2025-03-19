#include "bundle.h"

#include <util/generic/utility.h>
#include <util/generic/yexception.h>
#include <util/generic/ylimits.h>

#include <util/string/printf.h>

using namespace NExtendedMx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMultiClassWithFilterBundle
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TMultiClassWithFilterBundle : public TBundleBase<TMultiClassWithFilterConstProto> {
public:
    TMultiClassWithFilterBundle(const NSc::TValue& scheme)
        : TBundleBase(scheme) {}

    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        FilterCalcer = LoadFormulaProto(Scheme().FilterCalcer(), formulasStoragePtr);
        MultiClassCalcer = LoadFormulaProto(Scheme().MultiClassCalcer(), formulasStoragePtr);

        Validate();
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);

        context.DbgLog() << "MultiClassWithFilter calculation started\n";

        const auto& params = Scheme().Params();
        auto filterRes = FilterCalcer->DoCalcRelevExtended(features, featuresCount, context);

        const auto& availableValues = *context.GetMeta().FeatureContext()[params.FeatureName()].AvailibleValues().GetRawValue();
        if (filterRes > params.FilterThreshold()) {
            auto mcRes = MultiClassCalcer->DoCalcRelevExtended(features, featuresCount, context);
            double minDistance = Max<double>();
            TString selectedClass;
            for (const auto& mcb : params.MultiClassBounds()) {
                if (!availableValues.DictEmpty() && !availableValues.Get(mcb.Key()).IsTrue()) {
                    context.DbgLog() << "skip not available class " << mcb.Key() << "\n";
                    continue;
                }
                double distance = ClassDistance(mcRes, mcb.Value());
                if (distance < minDistance) {
                    minDistance = distance;
                    selectedClass = mcb.Key();
                }
            }
            if (!selectedClass.empty()) {
                LogValue(context, FEATURE_LOG_PREF + ToString(params.FeatureName()), selectedClass);
                context.GetResult().FeatureResult()[params.FeatureName()].Result()->SetString(selectedClass);
                return filterRes;
            }
        }
        return 0.f;
    }

    size_t GetNumFeats() const override {
        return Max(FilterCalcer->GetNumFeats(), MultiClassCalcer->GetNumFeats());
    }

private:
    using TConstClassBounds = TMultiClassWithFilterConstProto::TParamsConst::TClassBoundsConst;

    static double ClassDistance(double val, const TConstClassBounds& bounds) {
        if (bounds.Min().IsNull() && bounds.Max().IsNull()) {
            return 0.0;
        } else if (bounds.Max().IsNull()) {
            return val > bounds.Min() ?  0.0 : bounds.Min() - val;
        } else if (bounds.Min().IsNull()) {
            return val < bounds.Max() ? 0.0 : val - bounds.Max();
        } else {
            return (bounds.Min() <= val && val <= bounds.Max()) ? 0.0 : Min(Abs(val - bounds.Min()), Abs(val - bounds.Max()));
        }
    }

    // value in [min, max)
    static bool InClassBounds(double val, const TConstClassBounds& bounds) {
        return (bounds.Min().IsNull() || val >= bounds.Min()) && (bounds.Max().IsNull() || val < bounds.Max());
    }

    void Validate() const {
        for (const auto& categ2bounds : Scheme().Params().MultiClassBounds()) {
            // check null bounds
            const auto& bounds = categ2bounds.Value();
            const auto& categ = categ2bounds.Key();
            Y_ENSURE(!bounds.Min().IsNull() || !bounds.Max().IsNull(), Sprintf("Null bounds for [%s]", categ.data()));

            // check order relation
            Y_ENSURE(bounds.Min().IsNull() || bounds.Max().IsNull() || bounds.Min() < bounds.Max(),
                    Sprintf("[%s] max less or equal min", categ.data()));

            // check bounds intersection
            for (const auto& otherCateg2bounds : Scheme().Params().MultiClassBounds()) {
                const auto& otherBounds = otherCateg2bounds.Value();
                const auto& otherCateg = otherCateg2bounds.Key();
                if (categ == otherCateg) {
                    continue;
                }
                Y_ENSURE(otherBounds.Max().IsNull() || !InClassBounds(otherBounds.Max() - std::numeric_limits<double>::epsilon(), bounds),
                        Sprintf("[%s] max lies in [%s]", otherCateg.data(), categ.data()));
                Y_ENSURE(otherBounds.Min().IsNull() || !InClassBounds(otherBounds.Min() + std::numeric_limits<double>::epsilon(), bounds),
                        Sprintf("[%s] min lies in [%s]", otherCateg.data(), categ.data()));
            }
        }
    }

private:
    TExtendedRelevCalcer* FilterCalcer = nullptr;
    TExtendedRelevCalcer* MultiClassCalcer = nullptr;

};


TExtendedCalculatorRegistrator<TMultiClassWithFilterBundle> MultiClassPredictRegistrator("mc_with_filter");
