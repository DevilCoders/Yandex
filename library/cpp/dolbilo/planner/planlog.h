#pragma once

#include "loader.h"

class TPlanLoader: public IReqsLoader {
    public:
        TPlanLoader();
        ~TPlanLoader() override;

        void Process(TParams* params) override;

        IReqsLoader* Clone() const override {
            return new TPlanLoader(*this);
        }
};
