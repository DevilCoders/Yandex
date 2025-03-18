#include "sign.h"

#include <util/system/user.h>

namespace NSS {
    TLazySignerPtr SignByRsaKey(const TString& keyPath) {
        return std::make_unique<TCustomRsaKeyPolicy>(keyPath);
    }

    std::vector<TLazySignerPtr> SignByAny(const TMaybe<TStringBuf>& username) {
        const TString user = username ? TString{*username} : GetUsername();

        std::vector<TLazySignerPtr> signers;
        signers.reserve(2);

        signers.push_back(std::make_unique<TAgentPolicy>(user));
        signers.push_back(std::make_unique<TStandardRsaKeyPolicy>());

        return signers;
    }
}
