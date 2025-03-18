#pragma once

#include <util/generic/string.h>

struct evp_pkey_st;
typedef struct evp_pkey_st EVP_PKEY;

class TSSHPrivateKey {
public:
    enum class EPss {
        Use,
        Skip
    };

    enum class EAgent {
        Simulate,
        Skip
    };

    explicit TSSHPrivateKey(const TString& filename);

    TString Sign(const TStringBuf data, EPss usePss = EPss::Skip, EAgent agent = EAgent::Skip) const;

private:
    class TEvpPkeyDestroyer {
    public:
        static void Destroy(EVP_PKEY* evp);
    };

private:
    static unsigned char* PrepareBuffer(TString& str, size_t len, EAgent agent);

private:
    THolder<EVP_PKEY, TEvpPkeyDestroyer> Pkey_;
};
