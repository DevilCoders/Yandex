#pragma once

#include <kernel/common_server/ciphers/abstract.h>
#include <kernel/common_server/ciphers/config.h>

namespace NCS {
    class TAESCipher: public IKeyCipher {
    private:
        using TBase = IKeyCipher;

    protected:
        IKeyCipher::TPtr CreateNewVersionWithKey() const override {
            return MakeAtomicShared<TAESCipher>(GetPendingKeyUnsafe());
        }

    public:
        virtual bool Encrypt(const TString& plain, TString& encrypted) const override;
        virtual bool Decrypt(const TString& encrypted, TString& result) const override;

        using TBase::TBase;
    };

    class TAesKeyCipherConfig: public IKeyCipherConfig {
    private:
        using TBase = IKeyCipherConfig;
        static TFactory::TRegistrator<TAesKeyCipherConfig> Registrator;

    public:
        virtual IAbstractCipher::TPtr DoConstruct(const IBaseServer* /*server*/) const override {
            return MakeAtomicShared<TAESCipher>(GetKey(), GetPendingKey());
        }
        using TBase::TBase;
    };

    class TAESGcmCipher: public IKeyCipher {
    private:
        using TBase = IKeyCipher;

    protected:
        IKeyCipher::TPtr CreateNewVersionWithKey() const override {
            return MakeAtomicShared<TAESGcmCipher>(GetPendingKeyUnsafe());
        }

    public:
        virtual bool Encrypt(const TString& plain, TString& encrypted) const final;
        virtual bool Decrypt(const TString& encrypted, TString& result) const final;

        using TBase::TBase;
    };

    class TAesGcmKeyCipherConfig: public IKeyCipherConfig {
    private:
        using TBase = IKeyCipherConfig;
        static TFactory::TRegistrator<TAesGcmKeyCipherConfig> Registrator;

    public:
        virtual IAbstractCipher::TPtr DoConstruct(const IBaseServer* /*server*/) const override {
            return MakeAtomicShared<TAESGcmCipher>(GetKey(), GetPendingKey());
        }
        using TBase::TBase;
    };

    class TAesGcmKeyEncryptedCipher: public IKeyEncryptedCipher {
    private:
        using TBase = IKeyEncryptedCipher;

    protected:
        virtual IAbstractCipher::TPtr DoCreateNewVersion() const override {
            const auto newEncrypter = Encrypter->CreateNewVersion();
            TString newEncrypted(EncryptedKey.GetValue());
            if (newEncrypter && !Encrypter->ReEncrypt(newEncrypter, EncryptedKey.GetValue(), newEncrypted)) {
                return nullptr;
            }

            const auto newKeyCipher = KeyCipher->CreateNewVersionWithKey();
            return MakeAtomicShared<TAesGcmKeyEncryptedCipher>(!!newKeyCipher ? newKeyCipher : KeyCipher, !!newEncrypter ? newEncrypter : Encrypter, IKeyCipher::TCipherKey(newEncrypted, EncryptedKey.GetId()));
        }

    public:
        static TString GetKeyId(const TString& encrypted);
        using TBase::TBase;
    };
}
