#pragma once

#include <util/generic/string.h>
#include <library/cpp/ipreg/address.h>

namespace NIpreg {
    namespace NDetails {
        using TAddr = NIPREG::TAddress;

        struct TSubnet {
            TSubnet(const TString& ip = "")
                : TSubnet(ip, ip)
            {
            }

            TSubnet(const TString& high, const TString& low)
                : Lo(TAddr::ParseAny(low))
                , Hi(TAddr::ParseAny(high))
            {
            }

            bool operator<(const TSubnet& rhs) const {
                return Hi < rhs.Lo;
            }

            bool operator==(const TSubnet& rhs) const {
                return Lo == rhs.Lo && Hi == rhs.Hi;
            }

            inline bool operator!=(const TSubnet& rhs) const {
                return !(*this == rhs);
            }

            bool IsIn(const TAddr& addr) const {
                return Lo <= addr && addr <= Hi;
            }

            TAddr Lo;
            TAddr Hi;
        };
    }
}
