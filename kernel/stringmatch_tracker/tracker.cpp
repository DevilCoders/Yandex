#include "tracker.h"
#include "lcs_wrapper.h"
#include "trigram_wrappers.h"

namespace NStringMatchTracker {

    TStringMatchTracker::TStringMatchTracker(const TSimpleSharedPtr<IPreparer> preparer /* = nullptr*/)
        : Preparer(preparer)
    {
    }

    TStringMatchTracker::TStringMatchTracker(const TVector<TSimpleSharedPtr<ICalcer>>& calcerPtrs,
        const TSimpleSharedPtr<IPreparer> preparer /* = nullptr*/)
            : Calcers(calcerPtrs)
            , Preparer(preparer)
    {
    }

    void TStringMatchTracker::NewQuery(const TTextWithDescription& descr) {
        const TString preparedQuery = Preparer ? Preparer->Prepare(descr) : descr.GetText();
        for (auto& calcer : Calcers) {
            calcer->NewQuery(preparedQuery);
        }
    }

    void TStringMatchTracker::NewDoc(const TTextWithDescription& descr) {
        const TString preparedDoc = Preparer ? Preparer->Prepare(descr) : descr.GetText();
        for (auto& calcer : Calcers) {
            calcer->ProcessDoc(preparedDoc.begin(), preparedDoc.end());
        }
    }

    void TStringMatchTracker::ResetDoc() {
        for (auto& calcer : Calcers) {
            calcer->Reset();
        }
    }

    TVector<TFeatureDescription> TStringMatchTracker::CalcFeatures() const {
        TVector<TFeatureDescription> fv;
        for (const auto& calcer : Calcers) {
            calcer->CalcAllFeatures(fv);
        }
        return fv;
    }

    TStringMatchTracker& TStringMatchTracker::AddCalcer(TSimpleSharedPtr<ICalcer> calcerPtr) {
        Calcers.push_back(calcerPtr);
        return *this;
    }

    void TStringMatchTracker::SetPreparer(const TSimpleSharedPtr<IPreparer> preparer) {
        Preparer = preparer;
    }


    TVector<TSimpleSharedPtr<ICalcer>> TStringMatchTracker::CreateDefaultCalcers() {
        return {MakeSimpleShared<TLCSCalcer>(),
                MakeSimpleShared<TQueryTrigramOverDocCalcer>(),
                MakeSimpleShared<TDocTrigramOverQueryCalcer>()};
    }

}
