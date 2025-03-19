#pragma once

#include <kernel/common_server/ciphers/abstract.h>

namespace NCS {
    class ICipherWithReserve: public IAbstractCipher {
    private:
        using TBase = IAbstractCipher;

        bool OnMainEncrypt(const TString& plain, const TString& encrypted) const;

    protected:
        const IAbstractCipher::TPtr MainCipher;
        const IAbstractCipher::TPtr ReserveCipher;

        virtual bool Store(const TString& encrypted, const TString& reserveEncrypted = "") const = 0;
        virtual bool Restore(const TString& encrypted, TString& reserveEncrypted) const = 0;

    public:
        virtual bool Encrypt(const TString& plain, TString& encrypted) const final;

        virtual bool Decrypt(const TString& encrypted, TString& plain) const final;

        virtual bool NeedRotate() const override {
            return MainCipher->NeedRotate();
        }

        virtual TString GetVersion() const override {
            return MainCipher->GetVersion();
        }

        ICipherWithReserve(IAbstractCipher::TPtr main, IAbstractCipher::TPtr reserve)
            : MainCipher(main)
            , ReserveCipher(reserve)
        {
            CHECK_WITH_LOG(!!MainCipher);
            CHECK_WITH_LOG(!!ReserveCipher);
        }
    };
}
