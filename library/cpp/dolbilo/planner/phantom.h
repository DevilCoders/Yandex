#pragma once

#include "loader.h"

class TPhantomLoader: public IReqsLoader {
    public:
        TPhantomLoader();
        ~TPhantomLoader() override;

        TString Opts() override;
        bool HandleOpt(const TOption* option) override;
        void Process(TParams* params) override;
        void Usage() const override;

        IReqsLoader* Clone() const override {
            return new TPhantomLoader(*this);
        }

    private:
        TString Host;
        ui16 Port;
        TString DistrType;
        size_t Qps;
};
