#pragma once

#include <kernel/ethos/lib/reg_tree/compositions.h>
#include <kernel/extended_mx_calcer/factory/factory.h>
#include <kernel/matrixnet/mn_dynamic.h>
#include <kernel/matrixnet/mn_multi_categ.h>
#include <kernel/matrixnet/relev_calcer.h>
#include <kernel/catboost/catboost_calcer.h>

#include <util/generic/ptr.h>

namespace NFormulaStorageImpl {
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Load formula helpers
    //      if TCreatedCalcer is derived from TBase calcer:
    //          do nothing, simply move pointer
    //      else:
    //          call TBaseCalcer::Create with TCreatedCalcer* as argument
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <bool TIsDerived, typename TBaseCalcer, typename TCreatedCalcer>
    struct TCreateRelevCalcerPtrImpl {
        THolder<TBaseCalcer> operator()(THolder<TCreatedCalcer> calcer) const {
            return std::move(calcer);
        }
    };

    template <typename TBaseCalcer, typename TCreatedCalcer>
    struct TCreateRelevCalcerPtrImpl<false, TBaseCalcer, TCreatedCalcer> {
        THolder<TBaseCalcer> operator()(THolder<TCreatedCalcer> calcer) const {
            return TBaseCalcer::Create(std::move(calcer));
        }
    };

    template <typename TBaseCalcer, typename TCreatedCalcer>
    THolder<TBaseCalcer> CreateRelevCalcerPtrImpl(THolder<TCreatedCalcer> calcer) {
        constexpr bool isDerived = std::is_base_of<TBaseCalcer, TCreatedCalcer>::value;
        return TCreateRelevCalcerPtrImpl<isDerived, TBaseCalcer, TCreatedCalcer>()(std::move(calcer));
    }

    template <typename TBaseCalcer, typename TCreatedCalcer>
    THolder<TBaseCalcer> CreateRelevCalcerPtr(TCreatedCalcer* calcer) {
        return CreateRelevCalcerPtrImpl<TBaseCalcer, TCreatedCalcer>(THolder<TCreatedCalcer>(calcer));
    }

    template <typename TBaseCalcer, typename TLoader>
    THolder<TBaseCalcer> LoadFormulaImpl(IInputStream* in, const TLoader& loader) {
        return CreateRelevCalcerPtr<TBaseCalcer>(loader.Load(in));
    }

} // NFormulaStorageImpl

class TUnknownExtensionException: public yexception {
};

template <typename TBaseCalcer>
THolder<TBaseCalcer> LoadFormula(const TStringBuf extension, IInputStream* in) {
    using namespace NMatrixnet;
    using namespace NCatboostCalcer;
    using namespace NFormulaStorageImpl;
    using TExtendedLoaderFactory = NExtendedMx::TExtendedLoaderFactory;
    if (extension.EndsWith(".info")) {
        return LoadFormulaImpl<TBaseCalcer>(in, TRelevCalcerLoader<TMnSseDynamic>());
    } else if (extension.EndsWith(".mnmc")) {
        return LoadFormulaImpl<TBaseCalcer>(in, TRelevCalcerLoader<TMnMultiCateg>());
    } else if (extension.EndsWith(".regtree")) {
        return LoadFormulaImpl<TBaseCalcer>(in, NRegTree::TTCompactRegressionModelLoader());
    } else if (extension.EndsWith(TExtendedLoaderFactory::GetExtendedFileExtension())) {
        return LoadFormulaImpl<TBaseCalcer>(in, TExtendedLoaderFactory());
    } else if (extension.EndsWith(NCatboostCalcer::TCatboostCalcer::GetFileExtension())) {
        return LoadFormulaImpl<TBaseCalcer>(in, TRelevCalcerLoader<TCatboostCalcer>());
    }
    ythrow TUnknownExtensionException() << "unknown formula extension: " << extension;
}
