#pragma once
#include "calcer_base.h"
#include "feature_description.h"
#include "preparer_base.h"

#include <util/generic/ptr.h>

namespace NStringMatchTracker {

    class TStringMatchTracker {
    public:
        TStringMatchTracker(const TSimpleSharedPtr<IPreparer> preparer = nullptr);
        TStringMatchTracker(const TVector<TSimpleSharedPtr<ICalcer>>& calcerPtrs,
            const TSimpleSharedPtr<IPreparer> preparer = nullptr);

        void NewQuery(const TTextWithDescription& descr);
        void NewDoc(const TTextWithDescription& descr);
        void ResetDoc();

        TVector<TFeatureDescription> CalcFeatures() const;
        TStringMatchTracker& AddCalcer(TSimpleSharedPtr<ICalcer> calcerPtr);

        void SetPreparer(const TSimpleSharedPtr<IPreparer> preparer);

        static TVector<TSimpleSharedPtr<ICalcer>> CreateDefaultCalcers();

    private:
        TVector<TSimpleSharedPtr<ICalcer>> Calcers;
        TSimpleSharedPtr<IPreparer> Preparer;
    };

}
