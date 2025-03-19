#include "abstract.h"

#include <kernel/common_server/library/openssl/aes.h>

namespace NCS {
    bool ICipherWithReserve::OnMainEncrypt(const TString& plain, const TString& encrypted) const {
        TString reserveEncrypted;
        if (!ReserveCipher->Encrypt(plain, reserveEncrypted)) {
            return false;
        }
        if (!Store(encrypted, reserveEncrypted)) {
            return false;
        }
        return true;
    }

    bool ICipherWithReserve::Encrypt(const TString& plain, TString& encrypted) const {
        if (!MainCipher) {
            return false;
        }
        if (MainCipher->Encrypt(plain, encrypted)) {
            if (!OnMainEncrypt(plain, encrypted)) {
                return false;
            }
            return true;
        }
        if (!ReserveCipher || !ReserveCipher->Encrypt(plain, encrypted)) {
            return false;
        }
        if (!Store(encrypted)) {
            return false;
        }
        return true;
    }

    bool ICipherWithReserve::Decrypt(const TString& encrypted, TString& plain) const {
        if (!MainCipher) {
            return false;
        }
        if (MainCipher->Decrypt(encrypted, plain)) {
            return true;
        }
        TString reserveEncrypted;
        if (!ReserveCipher || !Restore(encrypted, reserveEncrypted)) {
            return false;
        }
        return ReserveCipher->Decrypt(reserveEncrypted, plain);
    }
}
