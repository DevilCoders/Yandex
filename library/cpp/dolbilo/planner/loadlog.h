#pragma once

#include "loader.h"

class TLoadLogLoader: public IReqsLoader {
    public:
        TLoadLogLoader();
        ~TLoadLogLoader() override;

        TString Opts() override;
        bool HandleOpt(const TOption* option) override;
        void Process(TParams* params) override;
        void Usage() const override;

        IReqsLoader* Clone() const override {
            return new TLoadLogLoader(*this);
        }

    private:
        TString Prefix_;
};
