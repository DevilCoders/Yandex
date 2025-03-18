#pragma once

#include "loader.h"

class TFullReqsLoader: public IReqsLoader {
    public:
        TFullReqsLoader();
        ~TFullReqsLoader() override;

        TString Opts() override;
        bool HandleOpt(const TOption* option) override;
        void Process(TParams* params) override;
        void Usage() const override;

        IReqsLoader* Clone() const override {
            return new TFullReqsLoader(*this);
        }

    private:
        TString Host;
        ui16 Port;
        TString DistrType;
        size_t Qps;
};
