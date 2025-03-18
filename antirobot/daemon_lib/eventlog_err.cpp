#include "eventlog_err.h"

#include "unified_agent_log.h"

#include <antirobot/idl/antirobot.ev.pb.h>

namespace NAntiRobot {

    TEvlogMessage::~TEvlogMessage() {
        if (Level > ANTIROBOT_DAEMON_CONFIG.LogLevel)
            return;

        if (Buf.Filled()) {
            if (Request || Header) {
                NAntirobotEvClass::TRequestGeneralMessage ev;
                if (Request)
                    ev.MutableHeader()->CopyFrom(Request->MakeLogHeader());
                else
                    ev.MutableHeader()->CopyFrom(*Header);
                ev.SetLevel(Level);
                ev.SetMessage(Buf.Data(), Buf.Filled());

                NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(ev);
                ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);
            } else {
                NAntirobotEvClass::TGeneralMessage ev;
                ev.SetLevel(Level);
                ev.SetMessage(Buf.Data(), Buf.Filled());

                NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(ev);
                ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);
            }
        }
    }

    NAntirobotEvClass::THeader MakeEvlogHeader(const TStringBuf& reqId, const TAddr& userAddr, const TUid& uid,
            const TStringBuf& yandexuid, const TAddr& partnerAddr, const TString& uniqueKey) {

        TTempBufOutput userAddrOut;
        userAddrOut << userAddr;
        TTempBufOutput partnerAddrOut;
        partnerAddrOut << partnerAddr;
        TTempBufOutput uidOut;
        uidOut << uid;

        NAntirobotEvClass::THeader header;
        header.SetReqid(reqId.data(), reqId.size());
        header.SetIpDeprecated(userAddr.AsIp());
        header.SetAddr(userAddrOut.Data(), userAddrOut.Filled());
        header.SetUidNs(uid.Ns);
        header.SetUidId(uid.Id);
        header.SetUid(uidOut.Data(), uidOut.Filled());
        header.SetYandexUid(yandexuid.data(), yandexuid.size());
        header.SetPartnerIpDeprecated(partnerAddr.AsIp());
        header.SetPartnerAddr(partnerAddrOut.Data(), partnerAddrOut.Filled());
        header.SetUniqueKey(uniqueKey);

        return header;
    }

    NAntirobotEvClass::THeader MakeEvlogHeader(const TAddr& userAddr)
    {
        return MakeEvlogHeader(TStringBuf(), userAddr, TUid::FromAddr(userAddr),
                               TStringBuf(), TAddr(), TString());
    }

}
