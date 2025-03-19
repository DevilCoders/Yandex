#pragma once

#include <util/generic/string.h>
#include <library/cpp/digest/old_crc/crc.h>

namespace NHerfHash {

inline ui32 HostHash(const TString& host);
inline ui32 OwnerHash(const TString& owner);

inline ui32 DefaultHerfHash(const TString& s)
{
    return crc32(s.data(), s.size());
}

ui32 HostHash(const TString& host)
{
    return DefaultHerfHash(host);
}

ui32 OwnerHash(const TString& owner)
{
    return DefaultHerfHash(owner);
}

}
