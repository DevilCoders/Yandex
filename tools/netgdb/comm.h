#pragma once

#include <quality/deprecated/Misc/commonStdAfx.h>

#include <util/string/cast.h>
#include <sys/wait.h>

#include <library/cpp/netliba/v12/udp_address.h>
#include <library/cpp/netliba/v12/udp_http.h>

extern TIntrusivePtr<NNetliba_v12::IRequester> Comm;
extern int RemotePortBase;
extern int HostIndexModule;
extern NNetliba_v12::TColors RegularColor;

inline int RemotePort (const TString& /*host*/) {
//    int hostIndex = 0;
//
//    for (size_t i = 0; i < host.length(); i++) {
//        if (host[i] <= '9' && host[i] >= '0')
//            hostIndex = hostIndex * 10 + (host[i] - '0');
//    }
    return RemotePortBase;// + (HostIndexModule > 0 ? hostIndex % (HostIndexModule) : 0);
}

void StartDaemon(const TString& storage);

class TOutputHandler {
public:
    virtual void Write(const char* buffer, int size) = 0;
};

class TBufferOutputHandler: public TOutputHandler {
public:
    TVector<char> Buffer;

    void Write(const char* buffer, int size) override
    {
        Buffer.insert(Buffer.end(), buffer, buffer + size);
    }
};


//int RunSHCommand(const TString& command, pid_t* pid = 0, TVector<char>* output = 0, bool createPG = false);
int RunSHCommand(const TString& command, const TString& wd = "", pid_t* pid = nullptr, bool createPG = false, TOutputHandler* oh = nullptr);

typedef TAutoPtr<TVector<char> > TPack;

inline TPack ConvertToPackage(const TString& str)
{
    return new TVector<char>(str.data(), str.data() + str.size());
}

inline TString Communicate(NNetliba_v12::TUdpAddress addr, TPack pack)
{
    TIntrusivePtr<NNetliba_v12::IRequester::TWaitResponse> req = Comm->WaitableRequest(addr, "", pack.Get());

    req->Wait();
    NNetliba_v12::TUdpHttpResponse *response = req->GetResponse();
    TString answer(response->Data.begin(), response->Data.end());
    delete response;
    return answer;
}

inline TString Communicate(NNetliba_v12::TUdpAddress addr, const TString& msg)
{
    TPack pack = ConvertToPackage(msg);
    return Communicate(addr, pack);
}

inline int CommunicateRC(NNetliba_v12::TUdpAddress addr, const TString& msg)
{
    TPack pack = ConvertToPackage(msg);
    TString m = Communicate(addr, pack);
    return m.size() ? atoi(m.data()) : -1;
}

inline void Reply(TGUID id, const TString& msg, NNetliba_v12::IRequester* comm = nullptr)
{
    Clog << GetGuidAsString(id) << "\t" << msg << Endl;
    if(id.IsEmpty())
        return;
    TPack pack = ConvertToPackage(msg);
    if (comm) {
        comm->SendResponse(id, pack.Get(), RegularColor);
    } else {
        Comm->SendResponse(id, pack.Get(), RegularColor);
    }
}

typedef ui64 time_ms;
time_ms GetTimeInMillis();

class TRequestSet
{
    TIntrusivePtr<NNetliba_v12::IRequestOps> Subreq;
    THashMap<TGUID, TString, TGUIDHash> Connections;

public:
    TRequestSet(TPtrArg<NNetliba_v12::IRequester> rq)
    {
        Subreq = rq->CreateSubRequester();
    }
    void SendRequest(const TString &host, const TString &url, TVector<char> *data)
    {
        NNetliba_v12::TUdpAddress addr = NNetliba_v12::CreateAddress(host.data(), RemotePort(host));
        Connections[Subreq->SendRequest(addr, url, data)] = host;
    }
    TAutoPtr<NNetliba_v12::TUdpHttpResponse> GetResponse(TString *resHost)
    {
        NNetliba_v12::TUdpHttpResponse *resp = Subreq->GetResponse();
        *resHost = "";
        if (resp) {
            THashMap<TGUID, TString, TGUIDHash>::iterator z = Connections.find(resp->ReqId);
            Y_VERIFY(z != Connections.end(), "unknown response");
            *resHost = z->second;
            Connections.erase(z);
        }
        return resp;
    }
    void Wait()
    {
        Subreq->GetAsyncEvent().Wait();
    }
    bool IsEmpty() const { return Connections.empty(); }
};

class IResultChecker
{
public:
    virtual ~IResultChecker() {}
    virtual bool Check(const TString& host, TVector<char>& result, TRequestSet *rs) = 0;
};

class TStringChecker: public IResultChecker {
    TString ControlPhrase;
public:
    TStringChecker(TString str)
        : ControlPhrase(str)
    {}

    bool Check(const TString&, TVector<char>& result, TRequestSet *rs) override
    {
        (void)rs;
        result.push_back(0);
        char* ptr = &result[0];
        while (ptr) {
            if(ControlPhrase == strsep(&ptr, "\t"))
                return true;
        }
        return false;
    }
};

bool CheckResult(const TVector<TString>& hosts, TVector<TString>* failed, const TString& command, IResultChecker* checker, time_ms to = 3000);

inline bool CheckResult(const TVector<TString>& hosts, TVector<TString>* failed, const TString& command, const TString& controlPhrase = "", time_ms to = 3000)
{
    TAutoPtr<TStringChecker> checker = new TStringChecker(controlPhrase);
    return CheckResult(hosts, failed, command, controlPhrase.empty() ? nullptr : checker.Get(), to);
}
