#pragma once
#include <kernel/common_server/signature/abstract/abstract.h>

namespace NCS {
    class TElSignatureMD5Config: public IElSignatureConfig {
    private:
        static TFactory::TRegistrator<TElSignatureMD5Config> Registrator;
    protected:
        virtual void DoInit(const TYandexConfig::Section* /*section*/) override {

        }
        virtual void DoToString(IOutputStream& /*os*/) const override {

        }
    public:
        virtual TAtomicSharedPtr<IElSignature> Construct(const IBaseServer& /*server*/) const override;
        static TString GetTypeName() {
            return "md5";
        }
        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };

    class TElSignatureMD5: public IElSignature {
    public:
        virtual bool MakeSignature(const TString& originalData, TString& signature) const override;
    };

    class TElSignatureXMLMD5Config: public IElSignatureConfig {
    private:
        static TFactory::TRegistrator<TElSignatureXMLMD5Config> Registrator;
    protected:
        virtual void DoInit(const TYandexConfig::Section* /*section*/) override {

        }
        virtual void DoToString(IOutputStream& /*os*/) const override {

        }
    public:
        virtual TAtomicSharedPtr<IElSignature> Construct(const IBaseServer& /*server*/) const override;
        static TString GetTypeName() {
            return "xmlMd5";
        }
        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };

    class TElSignatureXMLMD5: public IElSignatureXML {
    public:
        virtual bool FillSignatureInfo(const TString& strForSignature, NXml::TNode& signatureInfo) const override;
        virtual bool MakeSignature(const TString& originalData, TString& signature) const override;
    };
}
