#include "notification.h"

#include "master.h"

#include <tools/clustermaster/common/log.h>
#include <tools/clustermaster/common/task_list.h>
#include <tools/clustermaster/common/util.h>

#include <devtools/yanotifybot/client/cpp/client.h>

#include <library/cpp/http/simple/http_client.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/uri/encode.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/system/shellcommand.h>

#include <cstdio>

using namespace NAsyncJobs;

TString TSendUserNotifyJob::GetInstance() {
    TStringBuilder instance;
    if (MasterOptions.InstanceName.empty()) {
        instance << "Host <" << MasterEnv.SelfHostname << ">";
    } else {
        instance << "<" << MasterOptions.InstanceName << ">";
    }

    return instance;
}

TString TSendUserNotifyJob::GetShortDescription() {
    TStringBuilder description;
    description << "Target [" << Target.GetTarget() << "]";
    if (Type == FAILURE) {
        description << " failed.";
    } else if (Type == DEPFAILURE) {
        description << " got failed dependency.";
    }

    return description;
}

TString TSendMailJob::GetTitle() {
    return GetInstance() + " " + GetShortDescription();
}

TString TSendMailJob::GetMessage() {
    TStringBuilder message;
    message << "Target " << Target.GetTarget();

    ui32 port = MasterOptions.ROHTTPPort ? MasterOptions.ROHTTPPort : MasterOptions.HTTPPort;
    TSimpleHttpClient masterHttpClient(MasterEnv.SelfHostname, port);
    TStringBuilder requestPath;

    if (Type == FAILURE) {
        message << " failed.\n";

        if (Target.GetTasks().size() == 1) {
            message << "Failed task ID: " << Target.GetOnlyTask().GetTask() <<
                " (task's worker is " << Target.GetOnlyTask().GetWorker() << ").\n" <<
                "Here is the log:\n\n";

            requestPath << "/proxy/" << Target.GetOnlyTask().GetWorker() << "/logs/" <<
                Target.GetTarget() << '/' << Target.GetOnlyTask().GetTask();
        } else { // More than one task
            message << "Failed tasks IDs: " << JoinTaskList(", ", Target.GetTasksIds()) <<
                " (they are global - not within one worker).";
        }
    } else if (Type == DEPFAILURE) {
        TVector<int> causeTaskIds = CauseTarget->GetTasksIds();
        message << " - one of dependencies failed.\nAffected task IDs: " << JoinTaskList(", ", Target.GetTasksIds())
            << " (they are global - not within one worker).\nOriginal failed target: " << CauseTarget->GetTarget()
            << ".\nFailed tasks IDs: " << JoinTaskList(", ", causeTaskIds) << "\nHere is task " << causeTaskIds[0] << " log:\n\n";

        requestPath << "/proxy/" << CauseTarget->GetTasks()[0].GetWorker() << "/logs/" <<
            CauseTarget->GetTarget() << '/' << causeTaskIds[0];
    }

    TStringStream masterResponse;
    masterHttpClient.DoGet(requestPath, &masterResponse);
    message << masterResponse.Str();

    return message;
}

void TSendMailJob::Process(void*) {
    static TMutex lock;
    TGuard<TMutex> guard(lock); // See CLUSTERMASTER-51. When I modified async jobs processing to have more than one thread I got
            // strange locks concerned with TPipeOutput/TPipeInput (i.e. with popen). wget in 'TPipeInput log(~logCommand)'
            // had default timeout (15 minutes) and while it was hanging other threads sending mail couldn't return from popen
            // (forked process was hanging after vfork in 'V' state). Main problem was master hanged on script reload and this problem
            // AFAIU was also caused by popen that is used on script reload to apply m4 filter.

    if (MasterOptions.NoUserNotify) {
        return;
    }

    TVector<TString> recipients;
    for (const auto& r : Recipients) {
        const auto& resolvedVar = TryResolveVariable(r, Variables);
        auto&& splittedRecipients = SplitRecipients(resolvedVar);
        if (!splittedRecipients.empty()) {
            for (auto& value: splittedRecipients) {
                if (value.find('@') == TString::npos) {
                    value += "@yandex-team.ru";
                }
                recipients.push_back(value);
            }
        }
    }
    if (!recipients.empty()) {
        try {
            TStringStream cmdLine;
            TShellCommandOptions commandOptions;
            commandOptions.SetCloseAllFdsOnExec(true);
            TStringStream commandInput;

            if (MasterOptions.SmtpServer == "localhost") {
                commandInput << "To: " << JoinSeq(",", recipients) << "\n";
                commandInput << "Subject: [ClusterMaster] " << GetTitle() << "\n";
                commandInput << "Content-type: text/plain\n\n";
                commandInput << GetMessage();

                cmdLine << "/usr/sbin/sendmail -oi -t";
                if (MasterOptions.MailSender) {
                    cmdLine << " -r " << MasterOptions.MailSender;
                }
            } else {
                commandInput << GetMessage();

                cmdLine << "/usr/bin/mailx -s \"" << GetTitle() << "\"" <<
                    " -S smtp=smtp://" << MasterOptions.SmtpServer <<
                    " -S ttycharset=UTF-8";
                if (MasterOptions.MailSender) {
                    cmdLine << " -S from=\"" << MasterOptions.MailSender << " (Clustermaster notification)\"";
                }
                cmdLine << " " << JoinSeq(",", recipients);
            }

            commandOptions.SetInputStream(&commandInput);

            LOG("Sending mail with: " << cmdLine.Str());
            TShellCommand sendMail(cmdLine.Str(), commandOptions);
            sendMail.Run();
            if (sendMail.GetExitCode().GetRef()) {
                ythrow yexception() << sendMail.GetError();
            }
        } catch (const yexception& e) {
            ERRORLOG("Error sending mail: " << e.what());
        }
    } else {
        ERRORLOG("Error sending mail: list of recipients is empty. Failed target: " << Target.GetTarget());
    }
}

TString TSendSmsJob::GetTitle() {
    ythrow yexception() << "Not intended to be called.";
}

TString TSendSmsJob::GetMessage() {
    TStringBuilder message;
    message << GetInstance() << "\n" << GetShortDescription();
    if (Type == DEPFAILURE) {
        message << "\n[" << CauseTarget->GetTarget() << "] failed.";
    }
    return message;
}

TString TSendSmsJob::GetPhoneByLogin(const TString& login) {
    Y_ENSURE(TvmClient, "No tvm client.");

    static const TString STAFF_API_SERVER = "http://staff-api.yandex-team.ru";
    TSimpleHttpClient staffHttpClient(STAFF_API_SERVER, 80);

    TKeepAliveHttpClient::THeaders staffApiHeaders;
    staffApiHeaders["X-Ya-Service-Ticket"] = TvmClient->GetServiceTicketFor("Staff");

    TStringBuilder staffApiRequest;
    staffApiRequest << "/v3/persons?login=" << login << "&_fields=phones.number,phones.kind&_one=1";

    TStringStream staffApiResponse;
    staffHttpClient.DoGet(staffApiRequest, &staffApiResponse, staffApiHeaders);

    NJson::TJsonValue staffApiResponseJson = NJson::ReadJsonTree(&staffApiResponse);
    TString phoneNumber;
    for (const NJson::TJsonValue& phone : staffApiResponseJson["phones"].GetArray()) {
        if (phone["kind"] == "common") {
            phoneNumber = phone["number"].GetString();
        }
    }

    if (phoneNumber.empty()) {
        ythrow yexception() << "No phone numbers found on staff for " << login;
    }

    return phoneNumber;
}

void TSendSmsJob::SendSms(const TString& phoneNumber) {
    Y_ENSURE(TvmClient, "No tvm client.");

    static const TString YA_SMS_SERVER = "http://sms.passport.yandex.ru";

    TSimpleHttpClient yaSmsHttpClient(YA_SMS_SERVER, 80);

    TKeepAliveHttpClient::THeaders yaSmsHeaders;
    yaSmsHeaders["X-Ya-Service-Ticket"] = TvmClient->GetServiceTicketFor("YaSms");

    TStringStream yaSmsRequest;
    yaSmsRequest << "/sendsms?sender=clustermaster&text=";
    NUri::TEncoder::Encode(yaSmsRequest, "ClusterMaster: " + GetMessage());
    yaSmsRequest << "&phone=";
    NUri::TEncoder::Encode(yaSmsRequest, phoneNumber);

    LOG("Requesting " << YA_SMS_SERVER << yaSmsRequest.Str());

    TStringStream yaSmsResponse;
    yaSmsHttpClient.DoGet(yaSmsRequest.Str(), &yaSmsResponse, yaSmsHeaders);
    if (yaSmsResponse.Str().find("<errorcode>") != TString::npos) {
        ythrow yexception() << yaSmsResponse.Str();
    }
}

void TSendSmsJob::Process(void*) {
    if (MasterOptions.NoUserNotify) {
        return;
    }

    static TMutex lock;
    TGuard<TMutex> guard(lock); // See CLUSTERMASTER-51.

    TVector<TString> recipients;
    for (const auto& r : Recipients) {
        const auto& resolvedVar = TryResolveVariable(r, Variables);
        const auto& splittedRecipients = SplitRecipients(resolvedVar);
        if (!splittedRecipients.empty()) {
            recipients.insert(recipients.end(), splittedRecipients.begin(), splittedRecipients.end());
        }
    }

    if (!recipients.empty()) {
        for (const TString& recipient : recipients) {
            int retries = 3;

            while (retries--) {
                try {
                    TString phoneNumber = GetPhoneByLogin(recipient);
                    SendSms(phoneNumber);
                    break;
                } catch (const yexception& e) {
                    ERRORLOG("Error sending sms: " << e.what());
                }
            }
        }
    } else {
        ERRORLOG("Error sending sms: list of recipients is empty. Failed target: " << Target.GetTarget());
    }
}

TString TSendJNSChannelJob::GetTitle() {
    return MasterOptions.InstanceName.empty() ? MasterEnv.SelfHostname : MasterOptions.InstanceName;
}

TString TSendJNSChannelJob::GetMessage() {
    ythrow yexception() << "Not intended to be called.";
}

void TSendJNSChannelJob::Process(void*) {
    if (MasterOptions.NoUserNotify) {
        return;
    }

    static TMutex lock;
    TGuard<TMutex> guard(lock); // See CLUSTERMASTER-51.

    TVector<TString> recipients;
    for (const auto& r : Recipients) {
        const auto& resolvedVar = TryResolveVariable(r, Variables);
        const auto& splittedRecipients = SplitRecipients(resolvedVar);
        if (!splittedRecipients.empty()) {
            recipients.insert(recipients.end(), splittedRecipients.begin(), splittedRecipients.end());
        }
    }

    if (recipients.empty()) {
        ERRORLOG("Error sending message through JNS channel: list of recipients is empty. Failed target: " << Target.GetTarget());
    } else {
        try {
            NJson::TJsonValue requestJson(NJson::JSON_MAP);
            requestJson["project"] = "clustermaster";
            requestJson["params"] = NJson::JSON_MAP;
            auto& params = requestJson["params"];
            params["host"] = MasterEnv.SelfHostname;
            params["instance"] = GetTitle();
            if (Type == FAILURE) {
                requestJson["template"] = "cm_fails";
                params["failed_target_name"] = Target.GetTarget();
            } else if (Type == DEPFAILURE) {
                requestJson["template"] = "cm_depfails";
                params["failed_target_name"] = CauseTarget->GetTarget();
                params["depfailed_target_name"] = Target.GetTarget();
            }

            // documentation: https://docs.yandex-team.ru/jns/
            TSimpleHttpClient httpClient("https://jns.yandex-team.ru", 443);
            TSimpleHttpClient::THeaders headers = {{"Authorization", "OAuth " + Token}};
            for (const auto& recipient : recipients) {
                TVector<TString> item;
                Split(recipient, "/", item);
                if (item.size() != 2) {
                    ERRORLOG("Bad JNS channel target_project/channel pair: " << recipient);
                    continue;
                }
                requestJson["target_project"] = item[0];
                requestJson["channel"] = item[1];

                TStringStream requestBody;
                requestBody << requestJson;

                TStringStream response;
                httpClient.DoPost("/api/messages/send_to_channel_json", requestBody.Str(), &response, headers);
            }
        } catch (const yexception& e) {
            ERRORLOG("Error sending JNS channel message: " << e.what());
        }
    }
}

TString TSendTelegramJob::GetTitle() {
    ythrow yexception() << "Not intended to be called.";
}

TString TSendTelegramJob::GetMessage() {
    TString message;
    if (Type == FAILURE) {
        message += "Target <b>" + Target.GetTarget() + "</b> failed";
    } else if (Type == DEPFAILURE) {
        message += "\nTarget <b>" + Target.GetTarget() + "</b> got failed dependency:\n<b>" + CauseTarget->GetTarget() + "</b> failed.";
    }
    return message;
}

void TSendTelegramJob::Process(void*) {
    if (MasterOptions.NoUserNotify) {
        return;
    }

    static TMutex lock;
    TGuard<TMutex> guard(lock); // See CLUSTERMASTER-51.

    TVector<TString> recipients;
    for (const auto& r : Recipients) {
        const auto& resolvedVar = TryResolveVariable(r, Variables);
        const auto& splittedRecipients = SplitRecipients(resolvedVar);
        if (!splittedRecipients.empty()) {
            recipients.insert(recipients.end(), splittedRecipients.begin(), splittedRecipients.end());
        }
    }

    if (!recipients.empty()) {
        try {
            // documentation: https://wiki.yandex-team.ru/yatool/notify/
            const auto& sendMessageResult = NYaNotify::SendMessage(TelegramSecret, GetMessage(), "HTML", recipients);
            LOG("Telegram message has been sent successfully: " << sendMessageResult);
        } catch (const yexception& e) {
            ERRORLOG("Error sending telegram message: " << e.what());
        }
    } else {
        ERRORLOG("Error sending telegram message: list of recipients is empty. Failed target: " << Target.GetTarget());
    }
}

TString TPushJugglerEventJob::GetTitle() {
    return MasterOptions.InstanceName.empty() ? MasterEnv.SelfHostname : MasterOptions.InstanceName;
}

TString TPushJugglerEventJob::GetMessage() {
    TString message;
    if (Type == FAILURE) {
        message += "Target " + Target.GetTarget() + " failed";
    } else if (Type == DEPFAILURE) {
        message += "\nTarget " + Target.GetTarget() + " got failed dependency:\n" + CauseTarget->GetTarget() + " failed.";
    }
    return message;
}

void TPushJugglerEventJob::ProcessJugglerResponse(TStringStream* response) {
    NJson::TJsonValue responseJson = NJson::ReadJsonTree(response);
    if (responseJson["accepted_events"] != 1) {
        ERRORLOG("Juggler rejected pushed event with error [" << responseJson["events"][0]["error"] << "]");
    } else {
        LOG("Juggler event has been pushed successfully" << GetTitle());
    }
}

void TPushJugglerEventJob::Process(void*) {
    static TMutex lock;
    TGuard<TMutex> guard(lock); // See CLUSTERMASTER-51.

    if (MasterOptions.NoUserNotify || !MasterOptions.EnableJuggler) {
        return;
    }

    try {

        NJson::TJsonValue requestJson(NJson::JSON_MAP);
        requestJson["source"] = "clustermaster";

        NJson::TJsonValue event(NJson::JSON_MAP);

        event["host"] = GetTitle();
        event["service"] =  "clustermaster";
        event["description"] = GetMessage();
        event["status"] = "CRIT";
        event["tags"] = NJson::TJsonValue(NJson::JSON_ARRAY);
        event["tags"].AppendValue("target_failure");
        for (const TString& recipient : Recipients) {
            const auto& resolvedRecipient = TryResolveVariable(recipient, Variables);
            if (resolvedRecipient) {
                event["tags"].AppendValue(resolvedRecipient);
            }
        }

        requestJson["events"] = NJson::TJsonValue(NJson::JSON_ARRAY);
        requestJson["events"].AppendValue(event);

        TStringStream requestBody;
        requestBody << requestJson;
        TStringStream response;

        /* We need to sleep here because aggregated juggler check can forget previous CRIT
         * status only after 60 seconds. I tried send OK an then CRIT event, but it didn't
         * work well, because minimum aggregation period is 5 seconds, so you have to sleep
         * either way for juggler not to squash two events without noticing status change.
         * Also default aggregation period is 300 seconds, so I set ttl to 60 and refresh_time
         * to 5 seconds.
         * */
        Sleep(TDuration::Seconds(61));
        LOG("Pushing juggler event " << requestBody.Str());
        TSimpleHttpClient jugglerHttpClient("juggler-push.search.yandex.net", 80);
        jugglerHttpClient.DoPost("/events", requestBody.Str(), &response);
        ProcessJugglerResponse(&response);
    } catch (const yexception& e) {
        ERRORLOG("Error pushing juggler event: " << e.what());
    }
}
