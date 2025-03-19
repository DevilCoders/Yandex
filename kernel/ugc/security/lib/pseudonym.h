#pragma once

#include <kernel/ugc/security/lib/internal/crypto.h>
#include <kernel/ugc/security/lib/secret_manager.h>

namespace NUgc {
    namespace NSecurity {
        class TPseudonym {
        public:
            explicit TPseudonym(const TSecretManager& secretManager);

            TString IdToPseudo(const ui64 id) const;
            bool PseudoToId(const TStringBuf pseudo, ui64& id) const;

            static char GetVersion();

        private:
            NInternal::TBlowfish Cipher;
        };
    } // namespace NSecurity
} // namespace NUgc
