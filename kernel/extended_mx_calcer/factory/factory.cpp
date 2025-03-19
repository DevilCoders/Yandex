#include "factory.h"

#include <kernel/extended_mx_calcer/interface/based_on.h>

#include <library/cpp/scheme/scheme.h>

#include <util/ysaveload.h>
#include <util/folder/path.h>
#include <util/generic/yexception.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/stream/file.h>

namespace NExtendedMx {

    THolder<TExtendedRelevCalcer> CreateExtendedCalcer(const TString& key, const NSc::TValue& scheme) {
        TBasedOn<TBundleConstProto>::ValidateProtoThrow(scheme);
        auto calcer = TExtendedCalculatorFactory::Construct(key, scheme);
        if (!calcer) {
            ythrow yexception() << "unknown extended calcer type: " << key;
        }
        return THolder<TExtendedRelevCalcer>(calcer);
    }

    const TString& TExtendedLoaderFactory::GetExtendedFileExtension() {
        const static TString extension = ".xtd";
        return extension;
    }

    template <typename T>
    THolder<TExtendedRelevCalcer> LoadImpl(const NSc::TValue& scheme) {
        THolder<T> p(new T(scheme));
        return std::move(p);
    }

    TCalcerPtr TExtendedLoaderFactory::Load(const TBundleConstProto& bundleProto) const {
        return CreateExtendedCalcer(TString{*bundleProto.XtdType()}, *bundleProto.Value__);
    }

    TExtendedRelevCalcer* TExtendedLoaderFactory::Load(IInputStream *in) const {
        Y_ENSURE(in, "Absent input in bundle creation");
        auto mayBeCompressed = OpenMaybeCompressedInput(in);
        auto scheme = NSc::TValue::FromJsonThrow(mayBeCompressed->ReadAll());
        return Load(TBundleConstProto(&scheme)).Release();
    }

    TCalcerPtr TExtendedLoaderFactory::Load(const TString& path) const {
        Y_ENSURE(TFsPath(path).GetName().EndsWith(GetExtendedFileExtension()), "invalid blender_bundle suffix");
        TFileInput fin(path);
        return TCalcerPtr(Load(&fin));
    }

} // NExtendedMx
