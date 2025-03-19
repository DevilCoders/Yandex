#pragma once

#include <kernel/common_server/ciphers/abstract.h>

namespace NCS {
    class IPoolCipher: public IAbstractCipher {
    private:
        using TBase = IAbstractCipher;

    protected:
        virtual IAbstractCipher::TPtr GetCipherForEncryption() const = 0;
        virtual IAbstractCipher::TPtr GetCipherForDecryption(const TString& encrypted) const = 0;

    public:
        virtual bool Encrypt(const TString& plain, TString& encrypted) const final {
            auto cipher = GetCipherForEncryption();
            if (!cipher) {
                return false;
            }
            return cipher->Encrypt(plain, encrypted);
        }

        virtual bool Decrypt(const TString& encrypted, TString& plain) const final {
            auto cipher = GetCipherForDecryption(encrypted);
            if (!cipher) {
                return false;
            }
            return cipher->Decrypt(encrypted, plain);
        }

        virtual bool NeedRotate() const override {
            return false;
        }
    };
}
