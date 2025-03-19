#pragma once

#include <kernel/extended_mx_calcer/interface/extended_relev_calcer.h>
#include <kernel/extended_mx_calcer/proto/typedefs.h>

#include <library/cpp/object_factory/object_factory.h>

#include <util/generic/ptr.h>

namespace NSc {
    class TValue;
}

namespace NExtendedMx {

    using TCalcerPtr = THolder<TExtendedRelevCalcer>;

    class TExtendedLoaderFactory : public NMatrixnet::IRelevCalcerLoader {
    public:
        static const TString& GetExtendedFileExtension();
        TCalcerPtr Load(const TString& path) const;
        TCalcerPtr Load(const TBundleConstProto& bundleProto) const;
        TExtendedRelevCalcer* Load(IInputStream *in) const override;
    };

    using TExtendedCalculatorFactory = NObjectFactory::TParametrizedObjectFactory<
        TExtendedRelevCalcer, // result
        TString,               // key
        const NSc::TValue&    // args
    >;

    template <typename T>
    using TExtendedCalculatorRegistrator = TExtendedCalculatorFactory::TRegistrator<T>;

} // NExtendedMx
