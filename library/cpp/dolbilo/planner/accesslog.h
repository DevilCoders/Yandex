#pragma once

#include "loader.h"

class TAccessLogLoader: public IReqsLoader {
    public:
        TAccessLogLoader();
        ~TAccessLogLoader() override;

        TString Opts() override;
        bool HandleOpt(const TOption* option) override;
        void Process(TParams* params) override;
        void Usage() const override;

        IReqsLoader* Clone() const override {
            return new TAccessLogLoader(*this);
        }

    private:
        TString Format_;
        TString Host_;
        TString VirtualHost_;
        bool FixAuthCookie_;
};
