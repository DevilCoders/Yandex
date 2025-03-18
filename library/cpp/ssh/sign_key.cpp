#include "sign_key.h"

#include <library/cpp/openssl/holders/evp.h>

#include <contrib/libs/openssl/include/openssl/err.h>
#include <contrib/libs/openssl/include/openssl/evp.h>
#include <contrib/libs/openssl/include/openssl/ossl_typ.h>
#include <contrib/libs/openssl/include/openssl/pem.h>

#include <util/generic/yexception.h>

namespace {
    struct TFileHolder {
        FILE* File;

        TFileHolder(FILE* f)
            : File(f)
        {
        }

        ~TFileHolder() {
            fclose(File);
        }
    };

    struct TErrCleaner {
        TErrCleaner() {
            ERR_clear_error();
        }

        ~TErrCleaner() {
            ERR_clear_error();
        }
    };

    TStringBuf GetOpensslError() {
        return ERR_reason_error_string(ERR_get_error());
    }
}

void TSSHPrivateKey::TEvpPkeyDestroyer::Destroy(EVP_PKEY* evp) {
    EVP_PKEY_free(evp);
}

TSSHPrivateKey::TSSHPrivateKey(const TString& filename) {
    TErrCleaner cleaner;

    TFileHolder f = fopen(filename.c_str(), "r");
    Y_ENSURE(f.File, "Can't open file: " << filename);

    Pkey_.Reset(PEM_read_PrivateKey(f.File, nullptr, nullptr, nullptr));
    Y_ENSURE(Pkey_, "Failed to parse private ssh key: " << filename << ". " << GetOpensslError());

    int typeId = EVP_PKEY_base_id(Pkey_.Get());
    Y_ENSURE(typeId == EVP_PKEY_RSA,
             "Unsupported key type: " << OBJ_nid2sn(typeId));
}

TString TSSHPrivateKey::Sign(const TStringBuf data, EPss usePss, EAgent agent) const {
    TErrCleaner cleaner;
    NOpenSSL::TEvpMdCtx md_ctx;
    Y_ENSURE(EVP_DigestInit_ex(md_ctx, EVP_sha1(), nullptr) == 1,
             GetOpensslError());

    EVP_PKEY_CTX* pctx = nullptr;
    Y_ENSURE(EVP_DigestSignInit(md_ctx, &pctx, EVP_sha1(), nullptr, Pkey_.Get()) == 1,
             GetOpensslError());

    if (usePss == EPss::Use) {
        Y_ENSURE(EVP_PKEY_CTX_ctrl_str(pctx, "rsa_padding_mode", "pss") == 1,
                 GetOpensslError());
    }

    Y_ENSURE(EVP_DigestSignUpdate(md_ctx, data.data(), data.size()) == 1,
             GetOpensslError());

    size_t len = 0;
    Y_ENSURE(EVP_DigestSignFinal(md_ctx, nullptr, &len) == 1 && len,
             GetOpensslError());

    TString res;
    Y_ENSURE(EVP_DigestSignFinal(md_ctx, PrepareBuffer(res, len, agent), &len) == 1,
             GetOpensslError());

    return res;
}

static const char AGENT_RSA_PREFIX[] = {0x00, 0x00, 0x00, 0x07, 's', 's', 'h', '-', 'r', 's', 'a'};
unsigned char* TSSHPrivateKey::PrepareBuffer(TString& str, size_t len, TSSHPrivateKey::EAgent agent) {
    if (agent == EAgent::Skip) {
        str.resize(len);
        return (unsigned char*)str.data();
    }

    str.assign(AGENT_RSA_PREFIX, sizeof(AGENT_RSA_PREFIX));

    const size_t LEN_SIZE = 4;
    str.resize(len + sizeof(AGENT_RSA_PREFIX) + LEN_SIZE);

    for (size_t i = 0; i < LEN_SIZE; ++i) {
        str[sizeof(AGENT_RSA_PREFIX) + LEN_SIZE - 1 - i] = (len >> 8 * i) & 0xFF;
    }

    return (unsigned char*)str.data() + sizeof(AGENT_RSA_PREFIX) + LEN_SIZE;
}
