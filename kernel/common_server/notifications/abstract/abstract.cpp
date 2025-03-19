#include "abstract.h"
#include <library/cpp/logger/global/global.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/abstract/frontend.h>
#include <util/system/hostname.h>

IFrontendNotifier::TAlertInfoCollector IFrontendNotifier::AlertNotification(const bool doAssert) {
    return TAlertInfoCollector(doAssert);
}

void IFrontendNotifier::MultiLinesNotify(
    IFrontendNotifier::TPtr notifier,
    const TString& commonHeader,
    const TSet<TString>& reportLines,
    const TString& userId,
    const TContext& context) {
    TVector<TString> linesVector(reportLines.begin(), reportLines.end());
    MultiLinesNotify(notifier, commonHeader, linesVector, userId, context);
}

void IFrontendNotifier::MultiLinesNotify(
        IFrontendNotifier::TPtr notifier,
        const TString& commonHeader,
        const TVector<TString>& reportLines,
        const TString& userId,
        const TContext& context) {
    if (notifier) {
        TMessages reportMessages;
        for (auto line : reportLines) {
            reportMessages.emplace_back(MakeAtomicShared<IFrontendNotifier::TMessage>(line));
        }
        notifier->MultiLinesNotify(commonHeader, reportMessages, userId, context);
    } else {
        TStringStream ss;
        ss << commonHeader << Endl;
        for (ui32 rLine = 0; rLine < reportLines.size(); rLine++) {
            ss << ToString(rLine + 1) << "." << reportLines[rLine] << Endl;
        }
        TFLEventLog::Log(ss.Str(), TLOG_WARNING);
    }
}

void IFrontendNotifier::MultiLinesNotify(
        const TString& commonHeader,
        const TMessages& reportMessages,
        const TString& userId,
        const TContext& context) const {
    for (ui32 i = 0; i < reportMessages.size(); ) {
        size_t next = reportMessages.size();

        TStringStream ss = commonHeader;
        ss << Endl;

        for (ui32 rLine = i; rLine < next; ++rLine) {
            if (!reportMessages[rLine]) {
                continue;
            }
            const TString nextLine = ToString(rLine + 1) + "." + reportMessages[rLine]->GetBody();
            if (rLine > i && ss.Size() + nextLine.size() > 4000) {
                next = rLine;
                break;
            } else {
                ss << nextLine << Endl;
            }
        }
        i = next;

        TFLEventLog::Log(ss.Str(), TLOG_DEBUG);
        Notify(TMessage(ss.Str()), userId, context);
    }
}

IFrontendNotifier::TResult::TPtr IFrontendNotifier::Notify(const IFrontendNotifier::TMessage& message, const TString& userId, const TContext& context) const {
    if (!IsActive()) {
        TFLEventLog::Log("Not started notifier", TLOG_ERR)("name", NotifierName);
        return nullptr;
    }
    if (UseEventLog) {
        if (userId) {
            TFLEventLog::ModuleLog(NotifierName, TLOG_INFO)("data", message.SerializeToJson())("user_id", userId);
        } else {
            TFLEventLog::ModuleLog(NotifierName, TLOG_INFO)("data", message.SerializeToJson());
        }
    }

    IFrontendNotifier::TResult::TPtr res;
    if (0.95 * (message.GetBody().size() + message.GetHeader().size()) > GetMessageLengthLimit()) {
        const double correction = GetMessageLengthLimit() / (0.95 * (message.GetBody().size() + message.GetHeader().size()));
        TMessage messageCopy = TMessage(message.GetHeader().substr(0, Max<i32>(0, message.GetHeader().size() * correction - 3)) + "...",
                                        message.GetBody().substr(0, Max<i32>(0, message.GetBody().size() * correction - 3)) + "...");
        res = DoNotify(messageCopy, context);
    } else {
        res = DoNotify(message, context);
    }

    if (res && res->HasErrors()) {
        TFLEventLog::Log("Error while sending notification", TLOG_ERR)("result", res->SerializeToJson().GetStringRobust());
        TString eventName = NotifierName + "Error";
        TFLEventLog::ModuleLog(eventName, TLOG_INFO)("data", res->SerializeToJson());
    }
    return res;
}

IFrontendNotifier::TResult::TPtr IFrontendNotifier::Notify(IFrontendNotifier::TPtr notifier, const TString& message, const TString& userId, const TContext& context) {
    IFrontendNotifier::TResult::TPtr result;
    if (notifier) {
        result = notifier->Notify(IFrontendNotifier::TMessage(message), userId, context);
    } else {
        TFLEventLog::Notice(message);
    }
    return result;
}

bool IFrontendNotifier::SendDocument(IFrontendNotifier::TPtr notifier, const IFrontendNotifier::TMessage& message, const TString& mimeType) {
    if (notifier) {
        return notifier->SendDocument(message, mimeType);
    } else {
        return false;
    }
}

bool IFrontendNotifier::SendDocument(const IFrontendNotifier::TMessage& /*message*/, const TString& /*mimeType*/) const {
    TFLEventLog::Log("Document pusher is undefined", TLOG_ERR);
    return false;
}

bool IFrontendNotifier::SendPhoto(IFrontendNotifier::TPtr notifier, const IFrontendNotifier::TMessage& message) {
    if (notifier) {
        return notifier->SendPhoto(message);
    } else {
        return false;
    }
}

bool IFrontendNotifier::SendPhoto(const IFrontendNotifier::TMessage& /*message*/) const {
    TFLEventLog::Log("Photo pusher is undefined", TLOG_ERR);
    return false;
}

IFrontendNotifier::TAlertInfoCollector::~TAlertInfoCollector() {
    const TString& result = *this;
    TFLEventLog::Log(result, TLOG_ALERT);
    IFrontendNotifier::Notify(ICSOperator::GetServer().GetNotifier("internal-alerts-notifier"), result);
    Y_ASSERT(!DoAssert);
}
