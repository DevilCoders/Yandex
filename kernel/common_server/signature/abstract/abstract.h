#pragma once
#include <library/cpp/yconf/conf.h>
#include <util/stream/output.h>
#include <util/generic/ptr.h>
#include <kernel/common_server/library/interfaces/container.h>

namespace NXml {
    class TNode;
}

class IBaseServer;

namespace NCS {
    class IElSignature;

    class IElSignatureConfig {
    private:
        CSA_READONLY_DEF(TString, Id);
    protected:
        virtual void DoInit(const TYandexConfig::Section* section) = 0;
        virtual void DoToString(IOutputStream& os) const = 0;
    public:
        using TFactory = NObjectFactory::TObjectFactory<IElSignatureConfig, TString>;
        using TPtr = TAtomicSharedPtr<IElSignatureConfig>;
        virtual ~IElSignatureConfig() = default;
        virtual TString GetClassName() const = 0;
        virtual void Init(const TYandexConfig::Section* section) {
            Id = section->GetDirectives().Value("Id", Id);
            AssertCorrectConfig(!!Id, "empty signature id");
            DoInit(section);
        }
        virtual void ToString(IOutputStream& os) const {
            os << "Id: " << Id << Endl;
            DoToString(os);
        }
        virtual TAtomicSharedPtr<IElSignature> Construct(const IBaseServer& server) const = 0;
    };

    class TElSignatureConfigContainer: public TBaseInterfaceContainer<IElSignatureConfig> {
    private:
        using TBase = TBaseInterfaceContainer<IElSignatureConfig>;
    public:
        using TBase::TBase;
    };

    class TElSignaturesConfig {
    private:
        using TConfigs = TMap<TString, TElSignatureConfigContainer>;
        CSA_READONLY_DEF(TConfigs, Configs);
    public:
        void Init(const TYandexConfig::Section* section) {
            auto children = section->GetAllChildren();
            for (auto&& i : children) {
                if (i.first != "Signature") {
                    continue;
                }
                TElSignatureConfigContainer c;
                c.Init(i.second);
                AssertCorrectConfig(!!c, "incorrect config for signature");
                AssertCorrectConfig(Configs.emplace(c->GetId(), c).second, "signature ids duplication %s", c->GetId().data());
            }
        }
        void ToString(IOutputStream& os) const {
            for (auto&& [_, c] : Configs) {
                os << "<Signature>" << Endl;
                c.ToString(os);
                os << "</Signature>" << Endl;
            }
        }
    };

    class IElSignature {
    public:
        using TPtr = TAtomicSharedPtr<IElSignature>;
        virtual ~IElSignature() = default;
        virtual bool MakeSignature(const TString& originalData, TString& signature) const = 0;
    };

    class IElSignatureXML: public IElSignature {
    public:
        virtual bool FillSignatureInfo(const TString& strForSignature, NXml::TNode& signatureInfo) const = 0;
    };
}
