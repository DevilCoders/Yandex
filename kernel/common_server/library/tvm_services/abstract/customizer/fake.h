#pragma once
#include "abstract.h"

namespace NExternalAPI {

    class TFakeRequestCustomization: public IRequestCustomizer {
    private:
        static TFactory::TRegistrator<TFakeRequestCustomization> Registrator;
    public:
        static TString GetTypeName() {
            return "fake";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
        virtual void DoInit(const TYandexConfig::Section* /*section*/) override {

        }
        virtual void DoToString(IOutputStream& /*os*/) const override {

        }
    };
}
