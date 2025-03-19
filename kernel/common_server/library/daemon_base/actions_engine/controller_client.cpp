#include "controller_client.h"
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/http/io/stream.h>
#include <util/stream/str.h>
#include <util/network/socket.h>
#include <library/cpp/digest/md5/md5.h>

namespace NDaemonController {

    TString ASToString(const TAction::TStatus& status) {
        switch (status) {
        case TAction::asNotStarted: return "NOTSTARTED";
            case TAction::asInProgress: return "INPROGRESS";
            case TAction::asFail: return "FAIL";
            case TAction::asSuccess: return "SUCCESS";

            default:
                FAIL_LOG("Undefined behaviuor");
        }
        FAIL_LOG("Undefined behaviuor");
    }

    TAction::TStatus ASFromString(const TString& status) {
        if (status == "NOTSTARTED")
            return TAction::asNotStarted;
        if (status == "SUCCESS")
            return TAction::asSuccess;
        if (status == "INPROGRESS")
            return TAction::asInProgress;
        if (status == "FAIL")
            return TAction::asFail;
        FAIL_LOG("Undefined behaviuor");
    }

    class TRequestWaiter : public TControllerAgent::IRequestWaiter {
    public:
        TRequestWaiter(TSocket s, const TString& comment)
            : Socket(s)
            , Comment(comment)
        {}

        virtual bool WaitResult(TString& result) override {
            if (SOCKET(Socket) == INVALID_SOCKET)
                ythrow yexception() << Comment;
            TSocketInput si(Socket);
            THttpInput hi(&si);
            int ret = ParseHttpRetCode(hi.FirstLine());
            result = hi.ReadAll();
            bool success = ret == 200 || ret == 202;
            if (success)
                INFO_LOG << Comment << "status=finished;" << Endl;
            else
                ERROR_LOG << Comment << "status=failed;msg=" << result << Endl;
            return success;
        }

    private:
        TSocket Socket;
        TString Comment;
    };

    TControllerAgent::IRequestWaiter::TPtr SendRequest(const TString& host, ui16 port, const TString& data, TDuration connectionTimeout, TDuration timeout, const TString& comment) {
        try {
            TSocket s(TNetworkAddress(host, port), connectionTimeout);
            s.SetSocketTimeout(timeout.Seconds(), timeout.MilliSeconds() % 1000);
            TSocketOutput so(s);
            so << data;
            so.Flush();
            return new TRequestWaiter(s, comment);
        } catch (...) {
            TString newComment = comment + "status=failed;info=" + CurrentExceptionMessage() + ";";
            INFO_LOG << newComment << Endl;
            return new TRequestWaiter(INVALID_SOCKET, newComment);
        }
    }

    bool CheckPostData(const TString& postData, const TString& reply) {
        if (!postData)
            return true;
        NJson::TJsonValue replyJson;
        TStringInput si(reply);
        if (!NJson::ReadJsonTree(&si, &replyJson)) {
            ERROR_LOG << "reply is not json: " << reply << Endl;
            return false;
        }
        if (!replyJson.Has("content_hash")) {
            ERROR_LOG  << "reply does not contain content_hash: " << reply << Endl;
            return false;
        }
        TString origHash = MD5::Calc(postData);
        TString replyHash = replyJson["content_hash"].GetStringRobust();
        if (replyHash != origHash)
            ythrow yexception() << "wrong content hash: " << replyHash << " != " << origHash;
        return true;
    }

    TString GenerateRequest(const TString& command, const TString& postData, THttpMethod method) {
        TStringStream ss;
        if (method == hmAuto)
            method = !!postData ? hmPost : hmGet;
        ss << method << " /" << command << " HTTP/1.1\r\n";
        ss << "Accept-Encoding:*\r\n";
        if (!!postData) {
            ss << "Content-Length:" << postData.size() << "\r\n";
            ss << "Content-Type:application/octet-stream\r\n\r\n";
            ss << postData;
        } else {
            ss << "\r\n";
        }
        return ss.Str();
    }

    TControllerAgent::IRequestWaiter::TPtr TControllerAgent::ExecuteCommand(const TString& command, const TString& postData, TDuration connectionTimeout, TDuration socketTimeout, THttpMethod method) {
        TStringStream comment;
        comment << "actor=controller_agent " << Host << ":" << Port << ";command=" << command << ";";
        INFO_LOG << comment.Str() << "status=starting;" << Endl;
        return SendRequest(Host, Port, GenerateRequest(command, postData, method), connectionTimeout, socketTimeout, comment.Str());
    }

    bool TControllerAgent::ExecuteCommand(const TString& command, TString& result, ui32 connectionTimeoutMs, ui32 sendAttemptionsMax, const TString& postData, THttpMethod method) {
        TStringStream comment;
        comment << "actor=controller_agent " << Host << ":" << Port << ";command=" << command << ";";
        INFO_LOG << comment.Str() << "status=starting;" << Endl;
        try {
            TString request = GenerateRequest(command, postData, method);
            for (ui32 i = 0; (i < sendAttemptionsMax) && (!Callback || Callback->IsExecutable()); ++i) {
                try {
                    return SendRequest(Host, Port, request, TDuration::MilliSeconds(connectionTimeoutMs), TDuration::Seconds(1 + connectionTimeoutMs / 1000), comment.Str())->WaitResult(result);
                } catch (...) {
                    result = CurrentExceptionMessage();
                    INFO_LOG << comment.Str() << "body=" << request.substr(0, 256) << ";status=att_" << i << "_failed;message=" << result << Endl;
                    if (i != sendAttemptionsMax - 1)
                        Sleep(TDuration::Seconds(5));
                }
            }
            if (Callback && !Callback->IsExecutable()) {
                result = "Cancelled";
            }
            return false;
        }
        catch (...) {
            INFO_LOG << comment.Str() << "status=failed;info=" << CurrentExceptionMessage() << ";" << Endl;
            result = "Action failed on " + ToString() + ": " + CurrentExceptionMessage();
        }
        return false;
    }

    TString TAction::BuildCommand(TString& postData, const TString& uriPrefix) const {
        if (Status == asNotStarted) {
            TimeStart = Now();
            Status = asInProgress;
        }
        else
            VERIFY_WITH_LOG(Status == asInProgress, "Incorrect BuildCommand usage phase %d", (int)Status);
        TString command = uriPrefix + GetCustomUri() + "?" + DoBuildCommand() + "&parent_task=" + Parent + AdditionParams;
        GetPostContent(postData);
        return command;
    }

    void TAction::Fail(const TString& info) const {
        WARNING_LOG << "Action " << ActionName() << " failed: " << info << Endl;
        VERIFY_WITH_LOG(Status == asInProgress, "Incorrect fail status phase %d", (int)Status);
        TimeFinish = Now();
        Info = info;
        Status = asFail;
    }

    bool TAction::IsFailed() const {
        VERIFY_WITH_LOG(Status == asFail || Status == asSuccess, "Incorrect action %s processing", ActionName().data());
        return Status == asFail;
    }

    void TAction::Success(const TString& info) {
        VERIFY_WITH_LOG(Status == asInProgress, "Incorrect success status phase %d", (int)Status);
        INFO_LOG << "Action " << ActionName() << " finished: " << info << Endl;
        TimeFinish = Now();
        Info = info;
        Status = asSuccess;
    }

    NJson::TJsonValue TAction::SerializeExecutionInfo() const {
        NJson::TJsonValue result;
        result.InsertValue("action_name", ActionName());
        if (!!Parent)
            result.InsertValue("parent", Parent);
        NJson::TJsonValue& resultInfo = result.InsertValue("result", NJson::JSON_MAP);
        resultInfo.InsertValue("info", Info);
        resultInfo.InsertValue("status", ASToString(Status));
        resultInfo.InsertValue("is_finished", IsFinished());
        if (Status != asNotStarted)
            resultInfo.InsertValue("time_start", TimeStart.MilliSeconds());
        if (IsFinished())
            resultInfo.InsertValue("time_finish", TimeFinish.MilliSeconds());
        return result;
    }

    void TAction::DeserializeExecutionInfo(const NJson::TJsonValue& value) {
        NJson::TJsonValue::TMapType map;
        NJson::TJsonValue::TMapType mapResult;
        CHECK_WITH_LOG(value.GetMap(&map));
        NJson::TJsonValue::TMapType::const_iterator i = map.find("result");
        CHECK_WITH_LOG(i != map.end());
        CHECK_WITH_LOG(i->second.GetMap(&mapResult));
        i = map.find("parent");
        if (i != map.end())
            Parent = i->second.GetStringRobust();
        Info = mapResult["info"].GetStringRobust();
        Status = ASFromString(mapResult["status"].GetStringRobust());
        if (Status != asNotStarted)
            TimeStart = TInstant::MilliSeconds(mapResult["time_start"].GetIntegerRobust());
        if (IsFinished())
            TimeFinish = TInstant::MilliSeconds(mapResult["time_finish"].GetIntegerRobust());
    }

    NJson::TJsonValue TAction::Serialize() const {
        NJson::TJsonValue result = SerializeExecutionInfo();
        result.InsertValue("task", DoSerializeToJson());
        return result;
    }

    void TAction::Deserialize(const NJson::TJsonValue& value) {
        DeserializeExecutionInfo(value);
        NJson::TJsonValue::TMapType map;
        CHECK_WITH_LOG(value.GetMap(&map));
        NJson::TJsonValue::TMapType::const_iterator i = map.find("task");
        CHECK_WITH_LOG(i != map.end());
        DoDeserializeFromJson(i->second);
    }

    TString TAction::GetInfo() const {
        CHECK_WITH_LOG(IsFinished());
        return Info;
    }

    TAction::TPtr TAction::BuildAction(const NJson::TJsonValue& value) {
        NJson::TJsonValue::TMapType map;
        NJson::TJsonValue task;
        CHECK_WITH_LOG(value.GetMap(&map));
        CHECK_WITH_LOG(map.find("action_name") != map.end());
        TAction::TPtr result = TFactory::Construct(map["action_name"].GetStringRobust());
        VERIFY_WITH_LOG(!!result, "can't build action with name %s in task %s", map["action_name"].GetStringRobust().data(), value.GetStringRobust().data());
        result->Deserialize(value);
        return result;
    }

    TString TAction::GetCurrentInfo() const {
        TStringStream ss;
        ss << ActionName() << ":status=" << ASToString(Status) << ";";
        if (IsFinished())
            ss << "info=" << GetInfo() << ";";
        return ss.Str();
    }

    void TSimpleAsyncAction::AddPrevActionsResult(const NRTYScript::ITasksInfo& info) {
        if (!WaitActionName)
            return;
        if (Status != asNotStarted)
            return;
        NJson::TJsonValue status;
        if (!info.GetValueByPath(WaitActionName, "action.result.status", status)) {
            DEBUG_LOG << "Incorrect path " << WaitActionName + "-action.result.status" << Endl;
            return;
        }
        TStatus st = ASFromString(status.GetStringRobust());
        switch (st) {
        case asNotStarted:
        case asInProgress:
            return;
        case asSuccess:
            break;
        case asFail:
            TimeStart = Now();
            Status = asInProgress;
            Fail("Start task fail");
            break;
        default:
            FAIL_LOG("invalid status: %i", (int)st);
            break;
        }


        if (!IdTask) {
            NJson::TJsonValue result;
            if (info.GetValueByPath(WaitActionName, "action.id_task", result)) {
                TWriteGuard g(RWMutex);
                IdTask = result.GetStringRobust();
            } else {
                DEBUG_LOG << "Incorrect path " << WaitActionName << "-action.id_task" << Endl;
            }
        }
    }
}

template <>
void Out<NDaemonController::TAsyncPolicy>(IOutputStream& o, NDaemonController::TAsyncPolicy ap) {
    switch (ap) {
    case NDaemonController::apNoAsync: o << "N"; return;
    case NDaemonController::apStart: o << "S"; return;
    case NDaemonController::apWait: o << "W"; return;
    case NDaemonController::apStartAndWait: o << "SW"; return;
    default:
        FAIL_LOG("Invalid async policy: %i", (int)ap);
    }
}

template <>
NDaemonController::TAsyncPolicy FromStringImpl<NDaemonController::TAsyncPolicy, char>(const char* data, size_t len) {
    TStringBuf buf(data, len);
    if (buf == "N")
        return NDaemonController::apNoAsync;
    if (buf == "S")
        return NDaemonController::apStart;
    if (buf == "W")
        return NDaemonController::apWait;
    if (buf == "SW")
        return NDaemonController::apStartAndWait;
    ythrow yexception() << "Unknown async policy " << buf;
}

template <>
void Out<NDaemonController::THttpMethod>(IOutputStream& o, NDaemonController::THttpMethod hm) {
    switch (hm) {
    case NDaemonController::hmAuto: o << "Auto"; return;
    case NDaemonController::hmGet: o << "GET"; return;
    case NDaemonController::hmPost: o << "POST"; return;
    case NDaemonController::hmPut: o << "PUT"; return;
    case NDaemonController::hmDelete: o << "DELETE"; return;
    default:
        FAIL_LOG("Invalid http method : %i", (int)hm);
    }
}

template <>
NDaemonController::THttpMethod FromStringImpl<NDaemonController::THttpMethod, char>(const char* data, size_t len) {
    TStringBuf buf(data, len);
    if (buf == "Auto")
        return NDaemonController::hmAuto;
    if (buf == "GET")
        return NDaemonController::hmGet;
    if (buf == "POST")
        return NDaemonController::hmPost;
    if (buf == "PUT")
        return NDaemonController::hmPut;
    if (buf == "DELETE")
        return NDaemonController::hmDelete;
    ythrow yexception() << "Unknown http method " << buf;
}
