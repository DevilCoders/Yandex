#include "calcer_exception.h"
#include "extended_relev_calcer.h"

namespace NExtendedMx {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TExtendedRelevCalcer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    double TExtendedRelevCalcer::CalcRelevExtended(const TVector<float> &features, TCalcContext& context) const {
        return DoCalcRelevExtended(features.data(), features.size(), context);
    }

    THolder<TExtendedRelevCalcer> TExtendedRelevCalcer::Create(TCalcerPtr calcerPtr) {
        return MakeHolder<TExtendedRelevCalcerSimple>(std::move(calcerPtr));
    }

    double TExtendedRelevCalcer::DoCalcRelev(const float*) const {
        return 0;
    }

    const TMeta& TExtendedRelevCalcer::ViewMeta() const {
        return Meta;
    }

    TMeta& TExtendedRelevCalcer::GetMeta() {
        return Meta;
    }

    const TString& TExtendedRelevCalcer::GetAlias() const {
        static const TString& alias = "noalias";
        return alias;
    }

    const NSc::TValue& TExtendedRelevCalcer::ViewRoot() const {
        return NSc::TValue::DefaultValue();
    }

    void TExtendedRelevCalcer::Initialize(const ISharedFormulasAdapter* const formulasStoragePtr) {
        switch (InitStage) {
            case EInitStage::NoInit:
                InitStage = EInitStage::InitInProcess;
                InitializeAfterStorageLoad(formulasStoragePtr);
                InitStage = EInitStage::InitDone;
                break;
            case EInitStage::InitInProcess:
                InitStage = EInitStage::NoInit;
                ythrow TCalcerException(TCalcerException::GetCalcersCycleMessage());
            case EInitStage::InitDone:
                break;
        }
    }

    void TExtendedRelevCalcer::TryInitializeSubcalcer(const TCalcerSharedPtr& subcalcer, const TString& subcalcerName, const ISharedFormulasAdapter* const formulasStoragePtr) {
        try {
            subcalcer->Initialize(formulasStoragePtr);
        } catch (TCalcerException& exp) {
            InitStage = EInitStage::NoInit;
            exp.AddCalcerToErroredChain(subcalcerName);
            ythrow exp;
        }
    }

    void TExtendedRelevCalcer::AddChildCalcer(TCalcerConstSharedPtr childCalcer) {
        ChildrenCalcers.push_back(std::move(childCalcer));
    }

    void TExtendedRelevCalcer::AddStorageChildCalcerName(const TString& storageChildCalcerName) {
        StorageChildrenCalcersNames.push_back(storageChildCalcerName);
    }

    void TExtendedRelevCalcer::GetStorageChildrenCalcersNames(TSet<TString>& storageChildrenCalcersNames) const {
        storageChildrenCalcersNames.insert(StorageChildrenCalcersNames.cbegin(), StorageChildrenCalcersNames.cend());
        for (const auto& childCalcer : ChildrenCalcers) {
            if (const auto* childExtendedRelevCalcer = dynamic_cast<const TExtendedRelevCalcer*>(childCalcer.Get())) {
                childExtendedRelevCalcer->GetStorageChildrenCalcersNames(storageChildrenCalcersNames);
            }
        }
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TExtendedRelevCalcerSimple
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TExtendedRelevCalcerSimple::TExtendedRelevCalcerSimple(TExtendedRelevCalcerSimple::TCalcerPtr calcer)
        : Calcer(std::move(calcer)) {
        Y_ENSURE(Calcer, "calcer is nullptr");
    }

    double TExtendedRelevCalcerSimple::DoCalcRelev(const float* features) const {
        return Calcer->DoCalcRelev(features);
    }

    void TExtendedRelevCalcerSimple::DoCalcRelevs(const float* const* docsFeatures, double* resultRelev, const size_t numDocs) const {
        return Calcer->DoCalcRelevs(docsFeatures, resultRelev, numDocs);
    }

    void TExtendedRelevCalcerSimple::DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs) const {
        return Calcer->DoSlicedCalcRelevs(features, relevs, numDocs);
    }

    double TExtendedRelevCalcerSimple::DoCalcRelevExtended(const float* features, const size_t /*featuresCount*/, TCalcContext& /*context*/) const {
        return DoCalcRelev(features);
    }

    size_t TExtendedRelevCalcerSimple::GetNumFeats() const {
        return Calcer->GetNumFeats();
    }

    const NMatrixnet::TModelInfo* TExtendedRelevCalcerSimple::GetInfo() const {
        return Calcer->GetInfo();
    }

    const TExtendedRelevCalcer::TCalcerPtr::TValueType* TExtendedRelevCalcerSimple::Get() const {
        return Calcer.Get();
    }

    TExtendedRelevCalcer::TCalcerPtr::TValueType* TExtendedRelevCalcerSimple::Get() {
        return Calcer.Get();
    }

} // NExtendedMx
