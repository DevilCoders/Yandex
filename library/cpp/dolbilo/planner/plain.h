#pragma once

#include "loader.h"
#include "distribution.h"

class TPlainLoader: public IReqsLoader {
    public:
        TPlainLoader();
        ~TPlainLoader() override;

        TString Opts() override;
        bool HandleOpt(const TOption* option) override;
        void Process(TParams* params) override;
        void Usage() const override;

        IReqsLoader* Clone() const override {
            return new TPlainLoader(*this);
        }

    private:
        size_t Qps_;
        ui16 Port_;
        TString Host_;
        TString Headers_;
        TString DistrType_;
};
