#include "http.h"
#include <kernel/common_proxy/common/trace_replier.h>
#include <kernel/daemon/config/daemon_config.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/http/misc/httpcodes.h>

namespace NCommonProxy {

    class THttpSource::TOutMetaData : public TMetaData {
    public:
        TOutMetaData() {
            Register("cgi", dtSTRING);
            Register("post", dtBLOB);
            Register("uri", dtSTRING);
            Register("headers", dtSTRING);
            Register("remote_address", dtSTRING);
        }
    };

    THttpSource::THttpSource(const TString& name, const TProcessorsConfigs& configs)
        : TSource(name, configs)
        , Server(this, configs.Get<TConfig>(name)->GetHttpOptions())
    {}

    TClientRequest* THttpSource::CreateClient() {
        return new TClient(*this);
    }

    void THttpSource::Run() {
        VERIFY_WITH_LOG(Server.Start(), "Cannot start http server: %d / %s:%d", Server.GetErrorCode(), Server.Options().Host.data(), Server.Options().Port);
    }

    void THttpSource::DoStop() {
        Server.Shutdown();
    }

    void THttpSource::DoWait() {
        Server.Wait();
    }

    void THttpSource::CollectInfo(NJson::TJsonValue& result) const {
        TSource::CollectInfo(result);
        result["http_queue_size"] = Server.GetRequestQueueSize();
    }

    const NCommonProxy::TMetaData& THttpSource::GetOutputMetaData() const {
        return Default<TOutMetaData>();
    }

    THttpSource::TClient::TClient(const TSource& source)
        : Source(source)
    {}

    bool THttpSource::TClient::Reply(void* /*ThreadSpecificResource*/) {
        IReplier::TPtr replier = new TReplier(this, Source);
        try {
            ProcessHeaders();
            RD.Scan();
            if (RD.ScriptName() == TStringBuf("/ping")) {
                replier->AddReply(Source.GetName(), 200, "0;OK");
            } else {
                if (bool tr = false; RD.CgiParam.Has("trace") && TryFromString<bool>(RD.CgiParam.Get("trace"), tr) && tr) {
                    replier = MakeIntrusive<TTraceReplier>(replier);
                }
                TDataSet::TPtr data = MakeIntrusive<TDataSet>(Source.GetOutputMetaData());
                data->Set<TString>("cgi", RD.CgiParam.Print());
                data->Set<TBlob>("post", Buf);
                data->Set<TString>("uri", TString{RD.ScriptName()});
                TStringStream headers;
                Input().Headers().OutTo(&headers);
                data->Set<TString>("headers", headers.Str());
                data->Set<TString>("remote_address", TString{RD.RemoteAddr()});
                Source.Process(data, replier);
            }
        } catch (...) {
            replier->AddReply(Source.GetName(), 500, CurrentExceptionMessage());
        }
        return false;
    }

    THttpSource::TClient::~TClient() {
        try {
            ReleaseConnection();
        } catch (...) {
            ERROR_LOG << "error while destroy client: " << CurrentExceptionMessage() << Endl;
        }
    }

    THttpSource::TReplier::TReplier(TClient* client, const TSource& source)
        : TSource::TReplier(source, new TReporter(*this, client))
    {}

    THttpSource::TReplier::TReporter::TReporter(TSource::TReplier& owner, TClient* client)
        : TSource::TReplier::TReporter(owner)
        , Client(client)
    {}

    void THttpSource::TReplier::TReporter::Report(const TString& message) try {
        Client->Output() << "HTTP/1.1 " << HttpCodeStrEx(Owner.GetCode()) << "\r\n\r\n" << message;
        TSource::TReplier::TReporter::Report(message);
        Client.Reset();
    } catch (...) {
        ERROR_LOG << "cannot send reply: " << CurrentExceptionMessage() << Endl;
    }

    const THttpServer::TOptions& THttpSource::TConfig::GetHttpOptions() const {
        return HttpOptions;
    }

    bool THttpSource::TConfig::DoCheck() const {
        return true;
    }

    void THttpSource::TConfig::DoInit(const TYandexConfig::Section& componentSection) {
        TSource::TConfig::DoInit(componentSection);
        TYandexConfig::TSectionsMap sm = componentSection.GetAllChildren();
        auto i = sm.find("HttpServer");
        if (i == sm.end())
            ythrow yexception() << "HttpServer section not set";
        HttpOptions.Init(i->second->GetDirectives());
    }

    void THttpSource::TConfig::DoToString(IOutputStream& so) const {
        TSource::TConfig::DoToString(so);
        so << HttpOptions.ToString("HttpServer") << Endl;
    }

}
