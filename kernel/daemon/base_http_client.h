#pragma once

#include "info.h"

#include <library/cpp/http/misc/httpreqdata.h>
#include <library/cpp/http/server/http_ex.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/mediator/messenger.h>

class TSpecialServerInfoMessage: public NMessenger::IMessage {
private:
    const TString ScriptName;
    TString Report;
public:
    TSpecialServerInfoMessage(const TString& scriptName)
        : ScriptName(scriptName)
    {

    }

    const TString& GetScriptName() const {
        return ScriptName;
    }

    TString& GetReport() {
        return Report;
    }

    const TString& GetReport() const {
        return Report;
    }
};

struct TCommonHttpClientFeatures
{
    void ProcessInfoServer();
    void ProcessPing();

    virtual TServerInfo GetServerInfo() const;

    virtual void ProcessServerStatus();
    virtual void GetMetrics(IOutputStream& out) const;
    virtual void ProcessSuperMind(IOutputStream& out) const;
    virtual void ProcessTass(IOutputStream& out) const;

    // returns true if request was recognized as special one and no additional
    // processing required
    virtual bool ProcessSpecialRequest();

    virtual THttpOutput& Output() = 0;
    virtual const TCgiParameters& GetCgi() const = 0;
    virtual const TBaseServerRequestData& GetBaseRequestData() const = 0;
};

template <class T>
class TCommonHttpClient
    : public THttpClientRequestExtImpl<T>
    , public TCommonHttpClientFeatures
{
private:
    using THttpClient = THttpClientRequestExtImpl<T>;

public:
    const T& GetRequestData() const {
        return THttpClient::RD;
    }
    T& MutableRequestData() {
        return THttpClient::RD;
    }
    virtual THttpOutput& Output() final {
        return THttpClient::Output();
    }
    virtual const TCgiParameters& GetCgi() const final {
        return GetRequestData().CgiParam;
    }
    virtual const TBaseServerRequestData& GetBaseRequestData() const final {
        return GetRequestData();
    }
    virtual TBaseServerRequestData& MutableBaseRequestData() final {
        return MutableRequestData();
    }
};

using TBaseHttpClient = TCommonHttpClient<TServerRequestData>;
