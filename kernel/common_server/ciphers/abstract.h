#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/daemon/config/daemon_config.h>
#include <library/cpp/object_factory/object_factory.h>
#include <util/stream/file.h>
#include <util/string/hex.h>
#include <library/cpp/logger/global/global.h>

class IBaseServer;

namespace NCS {

    class IAbstractCipher {
    public:
        using TPtr = TAtomicSharedPtr<IAbstractCipher>;

    private:
        TString Name = "Unknown";

    protected:
        virtual TPtr DoCreateNewVersion() const = 0;

    public:
        virtual ~IAbstractCipher() = default;

        virtual bool Encrypt(const TString& plain, TString& encrypted) const = 0;
        virtual bool Decrypt(const TString& encrypted, TString& result) const = 0;
        virtual bool NeedRotate() const = 0;
        virtual TString GetVersion() const {
            return "default";
        }

        bool ReEncrypt(const IAbstractCipher::TPtr cipher, const TString& oldEncrypted, TString& newEncrypted) const {
            if (!cipher) {
                return false;
            }

            TString plain;
            if (!Decrypt(oldEncrypted, plain)) {
                return false;
            }
            if (!cipher->Encrypt(plain, newEncrypted)) {
                return false;
            }
            return true;
        }

        virtual bool ReencryptIfNeeded(TString& /*encrypted*/, bool& isReencrypted) const {
            isReencrypted = false;
            return true;
        }

        IAbstractCipher::TPtr CreateNewVersion() const {
            if (NeedRotate()) {
                return DoCreateNewVersion();
            }
            return nullptr;
        }

        template <class T>
        T* GetAsSafe() {
            return VerifyDynamicCast<T*>(this);
        }

        template <class T>
        const T* GetAsSafe() const {
            return VerifyDynamicCast<const T*>(this);
        }

        void SetName(const TString& name) {
            Name = name;
        }

        const TString& GetName() const {
            return Name;
        }
    };

    class IKeyCipher: public IAbstractCipher {
    public:
        using TPtr = TAtomicSharedPtr<IKeyCipher>;

        class TCipherKey {
        private:
            CSA_DEFAULT(TCipherKey, TString, Value);
            CSA_DEFAULT(TCipherKey, TString, Id);

        public:
            TCipherKey(const TString& value = "", const TString& id = "default")
                : Value(value)
                , Id(id)
            {
            }
        };

    private:
        CSA_DEFAULT(IKeyCipher, TCipherKey, Key);
        CSA_MAYBE(IKeyCipher, TCipherKey, PendingKey);

    protected:
        IAbstractCipher::TPtr DoCreateNewVersion() const override {
            return CreateNewVersionWithKey();
        }

    public:
        IKeyCipher(const TCipherKey& key, const TMaybeFail<TCipherKey>& pendingKey = Nothing())
            : Key(key)
            , PendingKey(pendingKey)
        {
        }

        virtual bool NeedRotate() const override {
            return !!PendingKey && !!PendingKey->GetValue();
        }

        virtual TPtr CreateNewVersionWithKey() const = 0;
    };

    class IKeyEncryptedCipher: public IAbstractCipher {
    protected:
        const IKeyCipher::TPtr KeyCipher;
        const IAbstractCipher::TPtr Encrypter;
        const IKeyCipher::TCipherKey EncryptedKey;

    public:
        using TPtr = TAtomicSharedPtr<IKeyEncryptedCipher>;

        IKeyEncryptedCipher(const IKeyCipher::TPtr keyCipher, const IAbstractCipher::TPtr encrypter, const IKeyCipher::TCipherKey& encryptedKey)
            : KeyCipher(keyCipher)
            , Encrypter(encrypter)
            , EncryptedKey(encryptedKey)
        {
            CHECK_WITH_LOG(Encrypter);
            CHECK_WITH_LOG(KeyCipher);
            KeyCipher->MutableKey().SetId(EncryptedKey.GetId());
        }

        virtual bool Encrypt(const TString& plain, TString& encrypted) const final {
            TString plainKey;
            if (!Encrypter->Decrypt(EncryptedKey.GetValue(), plainKey)) {
                return false;
            }
            KeyCipher->MutableKey().SetValue(plainKey);
            const bool r = KeyCipher->Encrypt(plain, encrypted);
            KeyCipher->MutableKey().SetValue("");
            return r;
        }

        virtual bool Decrypt(const TString& encrypted, TString& plain) const final {
            TString plainKey;
            if (!Encrypter->Decrypt(EncryptedKey.GetValue(), plainKey)) {
                return false;
            }
            KeyCipher->MutableKey().SetValue(plainKey);
            const bool r = KeyCipher->Decrypt(encrypted, plain);
            KeyCipher->MutableKey().SetValue("");
            return r;
        }

        virtual bool NeedRotate() const override {
            return KeyCipher->NeedRotate() || Encrypter->NeedRotate();
        }
    };
}
