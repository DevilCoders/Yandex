#include "ip2backend.h"

#include <antirobot/daemon_lib/antirobot_service.h>
#include <antirobot/daemon_lib/get_host_name_request.h>
#include <antirobot/idl/ip2backend.pb.h>
#include <antirobot/lib/addr.h>
#include <antirobot/lib/http_helpers.h>

#include <library/cpp/http/io/stream.h>
#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/hash_set.h>
#include <util/generic/xrange.h>
#include <util/generic/yexception.h>
#include <util/string/join.h>

namespace NAntiRobot {

THttpRequest MakeRequest(const TString& ipAddress, const THostAddr& antirobotService, int processorCount) {
    if (NAntiRobot::TAddr(ipAddress) == NAntiRobot::TAddr()) {
        throw yexception() << '"' << ipAddress << "\" isn't valid IPv4 or IPv6 address";
    }

    return NAntiRobot::CreateGetHostNameRequest(ToString(antirobotService), ipAddress, processorCount);
}

TString Ip2Backend(const THostAddr& antirobotService, const THttpRequest& request,
                  int sendCount)
{
    THashSet<TString> cachers;
    TVector<TString> processors;
    for (auto i : xrange(sendCount)) {
        Y_UNUSED(i);
        static NAntiRobot::TNehRequester nehRequester(10000);
        TString response = NAntiRobot::FetchHttpDataUnsafe(&nehRequester, antirobotService, request, TDuration::Seconds(1), "http");

        NIp2BackendProto::TResponse message;
        if (!message.ParseFromString(response)) {
            ythrow yexception() << "Corrupted response: " << response;
        }

        cachers.insert(message.GetCacher());

        TVector<TString> procList(message.processors().begin(), message.processors().end());
        if (processors.empty()) {
            processors = std::move(procList);
        } else if (processors != procList) {
            ythrow yexception() << "Something strange happened - got different processors list\n"
                                << "Previous: " << JoinSeq(" ", processors) << '\n'
                                << "Current: " << JoinSeq(" ", procList) << '\n';
        }
    }
    return "Cachers: " + JoinSeq(" ", cachers) + '\n' +
           "Processors: " + JoinSeq(" ", processors);
}

}
