#pragma once

#include "gazetteer_pool.h"

#include <kernel/remorph/common/load_options.h>
#include <kernel/remorph/proc_base/matcher_base.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NRemorphCompiler {

class TUnitConfig: public TSimpleRefCount<TUnitConfig> {
public:
    typedef TIntrusivePtr<TUnitConfig> TPtr;
    typedef TVector<TString> TDependencies;

public:
    TString Output;
    NMatcher::EMatcherType Type;

private:
    NRemorph::TFileLoadOptions LoadOptions;
    bool BaseDirExplicit;
    TVector<TString> Dependencies;
    TGazetteerPoolPtr GazetteerPool;

public:
    TUnitConfig(const TString& path, NMatcher::EMatcherType type);
    TUnitConfig(const TString& path, const TString& outputPath, NMatcher::EMatcherType type);

    const NRemorph::TFileLoadOptions& GetLoadOptions() const {
        return LoadOptions;
    }

    inline const TDependencies& GetDependencies() const {
        return Dependencies;
    }

    void SetOutput(const TString& outputPath);
    void SetGazetteer(const TString& gazetteerPath);
    void SetGazetteerBase(const TString& gazetteerBasePath);
    void SetGazetteerPool(TGazetteerPoolPtr gazetteerPool);
    void AddDependency(const TString& dependencyPath);
};

typedef TVector<TUnitConfig::TPtr> TUnits;

} // NRemorphCompiler
