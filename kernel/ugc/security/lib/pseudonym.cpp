#include "pseudonym.h"

#include <library/cpp/string_utils/base64/base64.h>

namespace NUgc {
    namespace NSecurity {
        static const char VERSION = '\001';
        // 1 byte for version + 8 bytes for encrypted passport id = 9
        // makes an even 12 in base64
        static const int PSEUDO_BYTE_LEN = 9;
        static const int PSEUDO_BASE_LEN = 12;

        TPseudonym::TPseudonym(const TSecretManager& secretManager)
            : Cipher(secretManager.GetKey()) {
        }

        TString TPseudonym::IdToPseudo(const ui64 id) const {
            char bytes[PSEUDO_BYTE_LEN];
            bytes[0] = GetVersion();
            *reinterpret_cast<ui64*>(bytes + 1) = Cipher.EcbEncrypt(id);
            return Base64EncodeUrl(TStringBuf(bytes, PSEUDO_BYTE_LEN));
        }

        bool TPseudonym::PseudoToId(const TStringBuf pseudo, ui64& id) const {
            if (pseudo.size() != PSEUDO_BASE_LEN) {
                return false;
            }
            char bytes[PSEUDO_BYTE_LEN];
            if (Base64StrictDecode(pseudo, bytes).size() != PSEUDO_BYTE_LEN) {
                return false;
            }
            if (bytes[0] != GetVersion()) {
                return false;
            }
            id = Cipher.EcbDecrypt(*reinterpret_cast<ui64*>(bytes + 1));
            return true;
        }

        char TPseudonym::GetVersion() {
            return VERSION;
        }
    } // namespace NSecurity
} // namespace NUgc
