#pragma once

#include <tools/clustermaster/common/constants.h>
#include <tools/clustermaster/common/messages.h>
#include <tools/clustermaster/common/worker_variables.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/list.h>
#include <util/generic/vector.h>
#include <util/network/pollerimpl.h>
#include <util/network/sock.h>
#include <util/stream/output.h>
#include <util/string/printf.h>
#include <util/system/guard.h>
#include <util/system/winint.h>

namespace NClusterMaster
{
enum EWorkerState {
    WS_DISCONNECTED = 0,
    WS_CONNECTING,
    WS_CONNECTED,
    WS_AUTHENTICATING,
    WS_INITIALIZING,
    WS_ACTIVE,
};

// template <>
// TString ToString<EWorkerState>(const EWorkerState& s);
class TLVBuffer: public TString {
public:
    TLVBuffer()
        : TString()
        , Type(0) {
    }

    TLVBuffer(int type)
        : TString()
        , Type(type) {
    }

    TLVBuffer(int type, size_t size)
        : TString(TString::TUninitialized(size))
        , Type(type) {
    }

    TLVBuffer(int type, const char *data, size_t size)
        : TString(data, size)
        , Type(type) {
    }

    void SetType(int type) {
        Type = type;
    }

    int GetType() {
        return Type;
    }

private:
    int Type;
};

typedef TAtomicSharedPtr<TLVBuffer> TLVBufferHandle;
static const size_t headerLength = 4;
typedef TAtomicSharedPtr<TInetStreamSocket> TSocketHandle;

template<class TSock, class TMtx>
class TWorkerCl: public TNonCopyable
{
public:
    TWorkerCl(const TString& hostname, ui32 wPort = CLUSTERMASTER_WORKER_PORT)
        : State(WS_DISCONNECTED)
        , Host(hostname)
        , Port(Sprintf("%d", wPort))
        , TotalDiskspace(0)
        , AvailDiskspace(0)
    {
        if (hostname.empty())
            ythrow yexception() << "attempt to create worker with empty hostname";

        struct addrinfo hints;
        struct addrinfo* result;

        Zero(hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int error = 0;
        if ((error = getaddrinfo(Host.data(), Port.data(), &hints, &result)) != 0)
            ythrow yexception() << "getaddrinfo(" << Host << "): " <<  gai_strerror(error);

        memcpy(SocketAddr.SockAddr(), result->ai_addr, result->ai_addrlen);

        freeaddrinfo(result);
    }
    virtual ~TWorkerCl()
    {
        Disconnect();
    }

    void Connect()
    {
        Disconnect();

        State = WS_CONNECTING;

        int err = 0;
        if ((err = Socket.Connect(SocketAddr)) != 0)
            ythrow yexception() << "connection to "  << SocketAddr.ToString() << " failed: " <<  LastSystemErrorText(err);

        //Socket->CheckSock();
        ConnectTime = TInstant::Now();
        State = WS_CONNECTED;
    }
    void Disconnect()
    {
        Socket.Disconnect();
        State = WS_DISCONNECTED;
    }
    //bool CheckConnect(); //not implemented yet
    void SetFailure(const TString& text) noexcept
    {
        FailureTime = TInstant::Now();
        FailureText = text;
    }

    //void SendSomeData();
    //void RecvSomeData(const TLockableHandle<TMasterGraph>::TWriteLock& graph);

    virtual IOutputStream& GetLogStream()
    {
        Cerr << TInstant::Now() << " " << Host << " ";
        return Cerr;
    }

    virtual void SendMessage(TLVBufferHandle ph) //can be overloaded to make sending in other thread
    {
        TGuard<TMtx> guard(WriteMutex);
        unsigned char hdr[headerLength];
        size_t size = ph->size();
        hdr[0] = (ui8)ph->GetType();
        hdr[1] = (size >> 16) & 0xff;
        hdr[2] = (size >> 8) & 0xff;
        hdr[3] = size & 0xff;
        Socket.WriteData(hdr, headerLength);
        Socket.WriteData(ph->data(), size);
    }

    virtual void OnMsg(const TErrorMessage& in)
    {
        if (in.GetFatal())
            ythrow yexception() << "Fatal error from worker: " << in.GetError();
        else
            SetFailure(in.GetError());
    }
    void SendAuthMesage(const TWorkerHelloMessage& in, const TString& authKP, bool masterIsSecondary = false)
    {
        Y_VERIFY(State == WS_CONNECTED, "wrong state");
        //if (!in.Check())
        //    ythrow yexception() << "worker version mismatch";

        TAuthReplyMessage message(masterIsSecondary);
        in.GenerateResponse(*message.MutableDigest(), authKP);
        //SendMessage(message.Pack());
        State = WS_AUTHENTICATING;
    }
    virtual void OnMsg(const TWorkerHelloMessage& in) = 0;
    virtual void OnMsg(const TAuthSuccessMessage&) = 0;
    /*
    {
        GetLogStream() << "Authorification success notification received, sending config" << Endl;

        // reply with new config
        TConfigMessage msg;
*/
        /*
        graph->ExportConfig(Host, &msg);

        EnqueueMessage(msg.Pack());

        State = WS_INITIALIZING;

        Revision.Up();
        */
    //}

    virtual void OnMsg(const TFullStatusMessage&)
    {
        GetLogStream() << "Full status received, processing, size" << Endl;

        /*
        graph->ProcessFullStatus(in, Host, Pool);
        */
        State = WS_ACTIVE;
    }
    virtual void OnMsg(const TSingleStatusMessage&)
    {
        GetLogStream() << "Single status received, processing" << Endl;
        //graph->ProcessSingleStatus(in, Host, Pool);
    }
    virtual void OnMsg(const TThinStatusMessage&)
    {
        GetLogStream() << "Thin status received, processing" << Endl;
        //graph->ProcessThinStatus(in, Host, Pool);
    }
    virtual void OnMsg(const TVariablesMessage& in)
    {
        GetLogStream() << "Variables message (" << in.FormatText() << ") received, processing" << Endl;

        if (State < WS_ACTIVE)
            Variables.Clear();

        Variables.Update(in);
    }
    virtual void OnMsg(const TDiskspaceMessage& in)
    {
        TotalDiskspace = in.GetTotal();
        AvailDiskspace = in.GetAvail();
    }
    virtual TLVBufferHandle RecvMessage() //can be overloaded for processing in other thread
    {
        unsigned char hdr[headerLength];
        Socket.ReadData(hdr, headerLength);
        TLVBufferHandle hndl = new TLVBuffer(); //two not needed new
        TLVBuffer& data = *hndl;
        data.SetType(hdr[0]);
        data.resize((int)hdr[3] | ((int)hdr[2] << 8) | ((int)hdr[1] << 16));
        Socket.ReadData(data.begin(), data.size());
        return hndl;
    }
    virtual void OnStatusChange() {}

    void ProcessMessage(TLVBufferHandle hndl)
    {
        TLVBuffer& data = *hndl;
        EWorkerState st = State;
#define SW_CASE_MSG(msg, valstate) case msg::Type : {msg m(hndl); if (valstate) OnMsg(m); else GetLogStream() << "wrong state " << (int)State << " for " << #msg << Endl; } break
        switch (data.GetType())
        {
            //SW_CASE_MSG(TErrorMessage, true);
            //SW_CASE_MSG(TWorkerHelloMessage, State == WS_CONNECTED);
            //SW_CASE_MSG(TAuthSuccessMessage, State == WS_AUTHENTICATING);
            //SW_CASE_MSG(TFullStatusMessage, State >= WS_INITIALIZING);
            //SW_CASE_MSG(TSingleStatusMessage, State == WS_ACTIVE);
            //SW_CASE_MSG(TThinStatusMessage, State == WS_ACTIVE);
            //SW_CASE_MSG(TVariablesMessage, State >= WS_INITIALIZING);
            //SW_CASE_MSG(TDiskspaceMessage, State >= WS_INITIALIZING);
        default:
            ythrow yexception() << "unknown message type received: " << (int)data.GetType();
        }
#undef SW_CASE_MSG
        if (st != State)
            OnStatusChange();
    }


public:
    EWorkerState GetState() const noexcept { return State; }

    const TString& GetHost() const noexcept { return Host; }
    const TString& GetPort() const noexcept { return Port; }

    TInstant GetConnectTime() const noexcept { return ConnectTime; }
    TInstant GetFailureTime() const noexcept { return FailureTime; }
    const TString& GetFailureText() const noexcept { return FailureText; }

    const TWorkerVariables& GetVariables() const noexcept { return Variables; }

    ui64 GetTotalDiskspace() const noexcept { return TotalDiskspace; }
    i64 GetAvailDiskspace() const noexcept { return AvailDiskspace; }

    const TSocketHandle& GetSocket() const noexcept { return Socket; }

    //TRevision::TValue GetRevision() const noexcept { return Revision; }

protected:
    TMtx WriteMutex;
    EWorkerState State;

    const TString Host;
    const TString Port;

    ui64 TotalDiskspace;
    i64 AvailDiskspace;

    TInstant ConnectTime;
    TInstant FailureTime;
    TString FailureText;

    TWorkerVariables Variables;

    TSockAddrInet SocketAddr;
    TSock Socket;
};

template<class TSock, class TMtx>
struct TWorkerClSimple: public TWorkerCl<TSock, TMtx>
{
    typedef TWorkerCl<TSock, TMtx> TBase;
    TWorkerClSimple(const TString& hostname, TString authKeyP, ui32 wPort = CLUSTERMASTER_WORKER_PORT)
        : TBase(hostname, wPort), AuthKeyPath(authKeyP) {}

    using TBase::OnMsg;
    void OnMsg(const TAuthSuccessMessage&)
    {
        TBase::GetLogStream() << "Authorification success notification received, sending config" << Endl;
        TBase::State = WS_ACTIVE;
    }
    void OnMsg(const TWorkerHelloMessage& in)
    {
        TBase::SendAuthMesage(in, AuthKeyPath, true);
    }

protected:
    TString AuthKeyPath;
};
}
