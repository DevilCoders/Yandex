#include "ip_list_pid.h"

#include "ip_interval.h"

#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/hex.h>
#include <util/string/strip.h>

namespace {

//! Parse project id notation as described here:
//! https://wiki.yandex-team.ru/noc/newnetwork/hbf/projectid/#faervolnajazapissetevogoprefiksasprojectidirabotasnim
bool ParseProjectId(TStringBuf str, ui32* projectId) {
    if (str.length() == 0 || str.length() > 8) {
        // Project id occupies 32 bits of address, so it must be between 1 and 8 hex digits.
        return false;
    }
    ui32 value = 0;
    for (const unsigned char c : str) {
        int digit = Char2DigitTable[c];
        if (digit == '\xff') {
            return false;
        }
        value <<= 4;
        value += digit;
    }
    *projectId = value;
    return true;
}

}

namespace NAntiRobot {

TIpListProjId::TIpListProjId(const TString& path) {
    Load(path);
}

void TIpListProjId::Append(IInputStream& input, bool check) {
    TString line;
    TVector<TIpInterval> intervals;

    while (input.ReadLine(line)) {
        TStringBuf l = StripString(TStringBuf(line).Before('#'));

        if (l.empty()) {
            continue;
        }

        if (l.Contains('@')) {
            try {
                TIpInterval interval = TIpInterval::Parse(l.After('@'));
                if (ui32 projectId; ParseProjectId(l.Before('@'), &projectId)) {
                    ProjectIds[interval].push_back(HostToInet(projectId));
                }
            } catch (...) {
                Cerr << CurrentExceptionMessage() << Endl;
            }
        } else {
            try {
                TIpInterval interval = TIpInterval::Parse(l);
                intervals.push_back(interval);
            } catch (...) {
                Cerr << CurrentExceptionMessage() << Endl;
            }
        }
    }

    if (check) {
        EnsureNoIntersections();
    }

    IpList.AddIntervals(intervals);
}

void TIpListProjId::Append(const TString& path, bool check) {
    if (path.empty()) {
        return;
    }

    TFileInput in(path);
    Append(in, check);
}

void TIpListProjId::Load(IInputStream& in) {
    IpList.Clear();
    ProjectIds.clear();
    Append(in);
}

void TIpListProjId::Load(const TString& fileName) {
    if (fileName.empty()) {
        return;
    }

    TFileInput in(fileName);
    Load(in);
}

void TIpListProjId::AddAddresses(const TVector<TAddr>& addresses) {
    IpList.AddAddresses(addresses);
}

void TIpListProjId::EnsureNoIntersections() const {
    if (ProjectIds.size() < 2) {
        return;
    }

    for (
        auto previous = ProjectIds.cbegin(), current = next(previous);
        current != ProjectIds.cend();
        ++previous, ++current
    ) {
        Y_ENSURE(
            previous->first.IpEnd < current->first.IpBeg,
            "Found intersection " << previous->first.Print() << " and " << current->first.Print()
        );
    }
}

bool TIpListProjId::Contains(const TAddr& addr) const {
    return addr.Valid() && (CheckProjectId(addr) || IpList.Contains(addr));
}

bool TIpListProjId::CheckProjectId(const TAddr& address) const {
    if (address.IsIp4() || ProjectIds.empty()) {
        return false;
    }

    auto it = ProjectIds.upper_bound(TIpInterval{address, address});
    if (it == ProjectIds.begin() && it->first.IpBeg != address) {
        return false;
    }

    if (it == ProjectIds.end() || it->first.IpBeg != address) {
        it--;
    }

    if (address < it->first.IpBeg || it->first.IpEnd < address) {
        return false;
    }

    for (auto projId : it->second) {
        if (projId == address.GetProjId()) {
            return true;
        }
    }

    return false;
}

} // namespace NAntiRobot
