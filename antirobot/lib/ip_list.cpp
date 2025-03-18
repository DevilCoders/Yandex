#include "ip_list.h"

#include "addr.h"
#include "ar_utils.h"

#include <util/generic/algorithm.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/strip.h>

namespace NAntiRobot {

    void TIpList:: Append(IInputStream& input) {
        TString line;

        while (input.ReadLine(line)) {
            TStringBuf l = StripString(TStringBuf(line).Before('#'));

            if (l.empty()) {
                continue;
            }

            try {
                TIpInterval interval = TIpInterval::Parse(l);
                IpIntervals.push_back(interval);
            } catch (...) {
                Cerr << CurrentExceptionMessage() << Endl;
            }
        }

        NormalizeIntervals();
    }

    void TIpList::Load(const TString& fileName) {
        if (fileName.empty()) {
            return;
        }

        TFileInput in(fileName);
        IpIntervals.clear();
        Append(in);
    }

    void TIpList::Load(IInputStream& in) {
        IpIntervals.clear();
        Append(in);
    }

    bool TIpList::Contains(const TAddr& addr) const {
        TIpIntervalVector::const_iterator toIpInterval = LowerBound(IpIntervals.begin(), IpIntervals.end(), addr);

        return (toIpInterval != IpIntervals.end() && toIpInterval->HasAddr(addr));
    }

    void TIpList::NormalizeIntervals() {
        Sort(IpIntervals.begin(), IpIntervals.end());

        TIpIntervalVector normalized;

        for (TIpIntervalVector::const_iterator iter = IpIntervals.begin(); iter != IpIntervals.end(); ++iter) {
            const TIpInterval& newInterval = *iter;

            if (normalized.empty()) {
                normalized.push_back(newInterval);
                continue;
            }

            TIpInterval& lastNormalized = normalized.back();

            if (newInterval.IpEnd <= lastNormalized.IpEnd)
                continue;

            // also glue together touching intervals
            if (newInterval.IpBeg <= lastNormalized.IpEnd.Next())
                lastNormalized.IpEnd = newInterval.IpEnd;
            else
                normalized.push_back(newInterval);
        }

        DoSwap(IpIntervals, normalized);
    }

    TString TIpList::Print() const {
        TString res;
        for (TIpIntervalVector::const_iterator it = IpIntervals.begin(); it != IpIntervals.end(); it++)
            res += it->Print() + '\n';
        return res;
    }

    void TIpList::Swap(TIpList& that) {
        IpIntervals.swap(that.IpIntervals);
    }

    void TIpList::AddIntervals(const TIpIntervalVector& intervals) {
        IpIntervals.reserve(IpIntervals.size() + intervals.size());
        std::copy(cbegin(intervals), cend(intervals), std::back_inserter(IpIntervals));
        NormalizeIntervals();
    }
}
