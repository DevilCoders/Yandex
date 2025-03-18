#include <nginx/modules/strm_packager/src/common/hmac.h>
#include <util/generic/yexception.h>

namespace NStrm::NPackager::NHmac {
    TString Hmac(const THmacType& type, const TString& secret, const TString& message) {
        std::shared_ptr<EVP_MD_CTX> ctx(EVP_MD_CTX_new(),
                                        [](EVP_MD_CTX* c) {
                                            EVP_MD_CTX_free(c);
                                        });
        Y_ENSURE(ctx.get());

        const EVP_MD* md = EVP_get_digestbyname(type.c_str());
        Y_ENSURE(md);

        std::shared_ptr<EVP_PKEY> pkey(EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL,
                                                            (const unsigned char*)secret.c_str(), secret.length()),
                                       [](EVP_PKEY* k) {
                                           EVP_PKEY_free(k);
                                       });
        Y_ENSURE(pkey.get());

        Y_ENSURE(EVP_DigestSignInit(ctx.get(), NULL, md, NULL, pkey.get()));
        Y_ENSURE(EVP_DigestSignUpdate(ctx.get(), message.c_str(), message.size()));

        TString tmp;
        size_t resultLen = 0;
        tmp.resize(EVP_MAX_MD_SIZE);
        Y_ENSURE(EVP_DigestSignFinal(ctx.get(), (unsigned char*)tmp.Data(), &resultLen));
        tmp.resize(resultLen);

        return tmp;
    }
}
