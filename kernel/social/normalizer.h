#pragma once

#include <kernel/social/protos/socnets.pb.h>

#include <util/generic/singleton.h>
#include <util/generic/hash.h>
#include <functional>

namespace NSocial {

class TIdentityNormalizer : TNonCopyable
{
    typedef std::function<TString(TString)> TNormFunction;
    TNormFunction Normalizers[NETWORKS_COUNT];

    TIdentityNormalizer();
public:
    Y_DECLARE_SINGLETON_FRIEND()
    static TIdentityNormalizer* Instance();
    TString Normalize(ESocialNetwork network, const TString& id);
};

}
