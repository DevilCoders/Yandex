#pragma once

#include <library/cpp/http/server/http.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/io/headers.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/folder/path.h>
#include <util/string/vector.h>
#include <kernel/common_server/library/logging/events.h>

class IReplyConstructor {
public:
    using TPtr = TAtomicSharedPtr<IReplyConstructor>;
    virtual ~IReplyConstructor() {}
    virtual bool GetReply(const TString& path, const NJson::TJsonValue& postJson, const TCgiParameters& cgi, THttpHeaders& headers, TString& content) const = 0;
    virtual ui16 GetCode(const TRequestReplier::TReplyParams& /*params*/) const {
        return HTTP_OK;
    }
};

class TBaseEmulatorServer : public THttpServer::ICallBack {
public:
    TBaseEmulatorServer() = default;
    void Run(ui16 httpPort);
    virtual TClientRequest* CreateClient() override;
    ~TBaseEmulatorServer();

    void RegisterReplier(const TString& name, TAtomicSharedPtr<IReplyConstructor> replier) {
        Repliers[name] = replier;
    }

    TAtomicSharedPtr<IReplyConstructor> GetReplier(const TString& name) const {
        auto it = Repliers.find(name);
        if (it == Repliers.end()) {
            return nullptr;
        }
        return it->second;
    }

private:
    THolder<THttpServer> Server;
    TMap<TString, TAtomicSharedPtr<IReplyConstructor>> Repliers;
};

class TEmulatorReplier : public TRequestReplier {
private:
    TBaseEmulatorServer& Owner;
public:
    TEmulatorReplier(TBaseEmulatorServer& owner)
            : Owner(owner)
    {
    }

protected:
    virtual bool DoReply(const TReplyParams& params) override {
        TParsedHttpFull parsedRequest(params.Input.FirstLine());
        TVector<TString> path = SplitString(TString(parsedRequest.Path), "/");
        TString api = path.empty() ? TString{} : path.front();

        auto replier = Owner.GetReplier(api);
        THttpHeaders headers;
        TString content;
        if (!replier) {
            MakeReply(params, headers, HTTP_BAD_GATEWAY, content);
            return true;
        }

        TString postData = params.Input.ReadAll();
        NJson::TJsonValue postJson = NJson::JSON_NULL;
        if (postData) {
            CHECK_WITH_LOG(NJson::ReadJsonFastTree(postData, &postJson, false));
        }

        TCgiParameters cgi;
        cgi.Scan(parsedRequest.Cgi);
        if (!replier->GetReply(TString(parsedRequest.Path), postJson, cgi, headers, content)) {
            TFLEventLog::Error("cannot make emulation reply");
            return false;
        }
        MakeReply(params, headers, replier->GetCode(params), content);
        return true;
    }

    void MakeReply(const TReplyParams& params, const THttpHeaders& headers, ui16 httpCode, const TString& content) {
        params.Output << "HTTP/1.1 "<< httpCode << Endl;
        for (auto&& i : headers) {
            params.Output << i.ToString() << Endl;
        }
        params.Output << Endl;
        params.Output << content;
    }
};
