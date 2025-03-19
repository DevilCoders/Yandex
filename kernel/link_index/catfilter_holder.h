#pragma once

#include <kernel/catfilter/catfilter_wrapper.h>

#include <library/cpp/deprecated/calc_module/simple_module.h>

class TCatFilterHolder : public TSimpleModule {
public:
    typedef THolder<ICatFilter> TICatFilterHolder;

private:
    const TString FilterObjFileName;
    TICatFilterHolder CatFilter;

public:
    TCatFilterHolder(const TString& filterObjFileName)
        : TSimpleModule("TCatFilterHolder")
        , FilterObjFileName(filterObjFileName)
    {
        Bind(this).To<const TICatFilterHolder*>(&CatFilter, "catfilter_output");
        Bind(this).To<&TCatFilterHolder::Init>("init");
    }

private:
    void Init() {
        CatFilter.Reset(GetCatFilter(FilterObjFileName, /*map = */ true, /*mirrors = */ nullptr));
    }

};
