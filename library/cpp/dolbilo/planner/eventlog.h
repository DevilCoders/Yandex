#pragma once

#include "loader.h"

class TEventLogLoader: public IReqsLoader {
    public:
        TEventLogLoader();
        ~TEventLogLoader() override;

        TString Opts() override;
        bool HandleOpt(const TOption* option) override;
        void Process(TParams* params) override;
        void Usage() const override;

        IReqsLoader* Clone() const override {
            return new TEventLogLoader(*this);
        }

    private:
        ui64 BeginDateTime;
        ui64 EndDateTime;
        TString SubstringFilter;
};
