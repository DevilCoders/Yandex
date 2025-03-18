#pragma once

#include "loader.h"

#include <util/stream/input.h>
#include <util/generic/ptr.h>

class TPcapDumpLoader: public IReqsLoader {
public:
    TPcapDumpLoader();
    ~TPcapDumpLoader() override;

    TString Opts() override;
    bool HandleOpt(const TOption* option) override;
    void Process(TParams* params) override;
    void Usage() const override;

    IReqsLoader* Clone() const override {
        return new TPcapDumpLoader(*this);
    }

private:
    TString Host_;
    ui16 Port_;
    ui16 ServerPort_;
    TString TimingsLog_;
};
