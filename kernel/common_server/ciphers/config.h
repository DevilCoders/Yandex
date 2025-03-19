#pragma once

#include <kernel/common_server/ciphers/abstract.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/daemon/config/daemon_config.h>
#include <library/cpp/object_factory/object_factory.h>
#include <util/stream/file.h>
#include <util/string/hex.h>
#include <library/cpp/logger/global/global.h>

class IBaseServer;

namespace NCS {

    class ICipherConfig {
    private:
        CSA_READONLY_DEF(TString, CipherName);

    protected:
        virtual void DoInit(const TYandexConfig::Section* /*section*/) {
        }
        virtual void DoToString(IOutputStream& /*os*/) const {
        }

        virtual IAbstractCipher::TPtr DoConstruct(const IBaseServer* server) const = 0;

    public:
        using TPtr = TAtomicSharedPtr<ICipherConfig>;
        using TFactory = NObjectFactory::TParametrizedObjectFactory<ICipherConfig, TString, TString>;
        ICipherConfig(const TString& cipherName)
            : CipherName(cipherName)
        {
        }
        virtual ~ICipherConfig() = default;

        IAbstractCipher::TPtr Construct(const IBaseServer* server) const {
            auto result = DoConstruct(server);
            if (!!result) {
                result->SetName(CipherName);
            }
            return result;
        }
        virtual void Init(const TYandexConfig::Section* section) final {
            CipherName = section->GetDirectives().Value("CipherName", CipherName);
            DoInit(section);
        }
        virtual void ToString(IOutputStream& os) const final {
            os << "CipherName: " << CipherName << Endl;
            DoToString(os);
        }
    };

    class IKeyCipherConfig: public ICipherConfig {
    private:
        using TBase = ICipherConfig;
        CSA_PROTECTED_DEF(IKeyCipherConfig, TString, KeyPath);
        CSA_PROTECTED_DEF(IKeyCipherConfig, TString, Key);
        CSA_PROTECTED_DEF(IKeyCipherConfig, TString, PendingKey);
        CSA_PROTECTED_DEF(IKeyCipherConfig, TString, PendingKeyPath);
        CSA_FLAG(IKeyCipherConfig, NeedHexDecode, true);

    public:
        void DoInit(const TYandexConfig::Section* section) override {
            KeyPath = section->GetDirectives().Value("KeyPath", KeyPath);
            if (KeyPath) {
                Key = Strip(TFileInput(KeyPath).ReadAll());
            }

            if (!Key) {
                Key = Strip(section->GetDirectives().Value("Key", Key));
            }

            PendingKeyPath = section->GetDirectives().Value("PendingKeyPath", PendingKeyPath);
            if (PendingKeyPath) {
                PendingKey = Strip(TFileInput(PendingKeyPath).ReadAll());
            }

            if (!PendingKey) {
                PendingKey = Strip(section->GetDirectives().Value("PendingKey", PendingKey));
            }

            NeedHexDecodeFlag = section->GetDirectives().Value("NeedHexDecode", NeedHexDecodeFlag);
            if (IsNeedHexDecode()) {
                Key = HexDecode(Key);
                PendingKey = HexDecode(PendingKey);
            }
        }

        void DoToString(IOutputStream& os) const override {
            os << "KeyPath: " << KeyPath << Endl;
            os << "PendingKeyPath: " << PendingKeyPath << Endl;
            os << "NeedHexDecode: " << NeedHexDecodeFlag << Endl;
        }

        using TBase::TBase;
    };
}
