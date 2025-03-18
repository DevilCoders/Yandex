#pragma once

#include "addr.h"
#include "ip_interval.h"

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/system/rwlock.h>

class IInputStream;

namespace NAntiRobot {
    class TIpList {
    public:
        using TIpIntervalVector = TVector<TIpInterval>;

    public:
        void Load(IInputStream& in);
        void Load(const TString& fileName);
        void Append(IInputStream& stream);

        bool Contains(const TAddr& addr) const;

        size_t Size() const {
            return IpIntervals.size();
        }

        TString Print() const;

        void AddAddresses(const TVector<TAddr>& addresses) {
            AddAddresses(addresses.begin(), addresses.end());
        }

        void AddIntervals(const TIpIntervalVector& intervals);

        void Swap(TIpList& that);
        void Clear() {
            IpIntervals.clear();
        }


        TIpIntervalVector::const_iterator begin() const {
            return IpIntervals.begin();
        }

        TIpIntervalVector::const_iterator end() const {
            return IpIntervals.end();
        }

    private:
        void NormalizeIntervals();

        template<class ForwardIterator>
        void AddAddresses(ForwardIterator begin, ForwardIterator end) {
            IpIntervals.reserve(IpIntervals.size() + std::distance(begin, end));
            for (ForwardIterator i = begin; i != end; ++i) {
                IpIntervals.push_back(TIpInterval(*i, *i));
            }
            NormalizeIntervals();
        }

    private:
        TIpIntervalVector IpIntervals;
    };
}
