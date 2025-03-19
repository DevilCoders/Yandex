#pragma once

#include <kernel/common_server/ciphers/abstract.h>
#include <kernel/common_server/ciphers/config.h>
#include <kernel/common_server/library/openssl/rsa.h>

namespace NCS {
    class TRSACipher: public IKeyCipher {
    private:
        using TBase = IKeyCipher;
        CSA_DEFAULT(TRSACipher, NOpenssl::TRSAPublicKey, PubKey);
        CSA_MAYBE(TRSACipher, NOpenssl::TRSAPrivateKey, PrivKey);

    protected:
        IKeyCipher::TPtr CreateNewVersionWithKey() const override {
            return MakeAtomicShared<TRSACipher>(GetPendingKeyUnsafe());
        }

    public:
        virtual bool Encrypt(const TString& plain, TString& encrypted) const override;
        virtual bool Decrypt(const TString& encrypted, TString& result) const override;

        void SetPrivateKey(const TString& privKeyStr);

        TRSACipher(const TCipherKey& key, const TMaybeFail<TCipherKey>& pendingKey = Nothing(), const TString& privKeyStr = "");
    };

    class TRSAKeyCipherConfig: public IKeyCipherConfig {
    private:
        using TBase = IKeyCipherConfig;
        static TFactory::TRegistrator<TRSAKeyCipherConfig> Registrator;

        CSA_READONLY_DEF(TString, PrivKeyPath);
        CSA_READONLY_DEF(TString, PrivKey);

    protected:
        void DoInit(const TYandexConfig::Section* section) override;
        void DoToString(IOutputStream& os) const override;

    public:
        virtual IAbstractCipher::TPtr DoConstruct(const IBaseServer* /*server*/) const override;
        using TBase::TBase;
    };
}
