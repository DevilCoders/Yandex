#pragma once

#include "sockhandler.h"

#include <library/cpp/http/fetch/exthttpcodes.h>
#include <library/cpp/uri/http_url.h>

#include <util/datetime/base.h>
#include <util/generic/utility.h>
#include <util/network/hostip.h>
#include <util/network/ip.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace fetch {

struct TNoTimer {
    inline void OnBeforeSend() { }
    inline void OnAfterSend() { }
    inline void OnBeforeRecv() { }
    inline void OnAfterRecv() { }
};

template <class TSockHndl = TSimpleSocketHandler,
          class TErrorLogger = TSslSocketBase::TFakeLogger,
          class TTimer = TNoTimer,
          template<class, class> class TSslSocketImpl = TSslSocketHandler>
class THttpsAgent : public TTimer
{
public:
    typedef TSslSocketImpl<TSockHndl, TErrorLogger> TSocket;
    THttpsAgent()
        : Socket(new TSocket)
        , Address(0)
        , Scheme(0)
        , Persistent(0)
        , Timeout(TDuration::MicroSeconds(150))
        , Hostheader(nullptr)
        , Footer(nullptr)
        , pHostBeg(nullptr)
        , pHostEnd(nullptr)
        , AltFooter(nullptr)
        , PostData(nullptr)
        , PostDataLen(0)
        , HostheaderLen(0)
    {
        SetIdentification("YandexSomething/1.0", "webadmin@yandex.ru");
    }

    ~THttpsAgent() {
        Disconnect();
        delete[] Hostheader;
        delete[] Footer;
    }

    void SetIdentification(const char* userAgent, const char* httpFrom) {
        Y_VERIFY(Socket.Get(), "HttpsAgent: socket is picked out. Can't use until a valid socket is set");
        delete[] Footer;
        size_t len = userAgent ? strlen(userAgent) + 15 : 0;
        len += httpFrom ? strlen(httpFrom) + 9 : 0;
        len += 3;
        Footer = new char[len];
        if (userAgent)
            strcat(strcat(strcpy(Footer, "User-Agent: "), userAgent), "\r\n");
        if (httpFrom)
            strcat(strcat(strcat(Footer, "From: "), httpFrom), "\r\n");
    }

    void SetUserAgentFooter(const char *altFooter) {
        AltFooter = altFooter;
    }

    void SetPostData(const char* postData, size_t postDataLen) {
        PostData = postData;
        PostDataLen = postDataLen;
    }

    TNetworkAddress GetAddress() const {
        return Address;
    }

    int GetScheme() const {
        return Scheme;
    }

    void SetTimeout(TDuration tim) {
        Timeout = tim;
    }

    int Disconnected() {
        return !Persistent || !Socket.Get() || !Socket->Good();
    }

    int SetHost(const char* hostname, TIpPort port, int scheme = THttpURL::SchemeHTTP) {
        Disconnect();
        Address = TNetworkAddress(hostname, port);
        Scheme = scheme;
        size_t reqHostheaderLen = strlen(hostname) + 20;
        if (HostheaderLen < reqHostheaderLen) {
            delete[] Hostheader;
            Hostheader = new char[(HostheaderLen = reqHostheaderLen)];
        }
        if (Scheme == THttpURL::SchemeHTTPS && port == 443 || port == 80)
            sprintf(Hostheader, "Host: %s\r\n", hostname);
        else
            sprintf(Hostheader, "Host: %s:%u\r\n", hostname, port);
        pHostBeg = strchr(Hostheader, ' ') + 1;
        pHostEnd = strchr(pHostBeg, '\r');
        // convert hostname to lower case since some web server don't like
        // uppper case (Task ROBOT-562)
        for (char *p = pHostBeg; p < pHostEnd; p++)
            *p = tolower(*p);
        SocketCtx.Host = pHostBeg;
        SocketCtx.HostLen = pHostEnd - pHostBeg;
        return 0;
    }

    void SetHostHeader(const char* host) {
        size_t reqHostheaderLen = strlen(host) + 20;
        if (HostheaderLen < reqHostheaderLen) {
            delete[] Hostheader;
            Hostheader = new char[(HostheaderLen = reqHostheaderLen)];
        }
        sprintf(Hostheader, "Host: %s\r\n", host);
    }

    void SetSocket(TSocket *s) {
        Y_VERIFY(s, "HttpsAgent: socket handler is null");
        SocketCtx.FreeBuffers();
        if (s->HasSsl())
            SocketCtx.AllocBuffers();
        Socket.Reset(s);
    }

    TSocket *PickOutSocket() {
        SocketCtx.FreeBuffers();
        SocketCtx.CachedSession.Destroy();
        return Socket.Release();
    }

    void Disconnect() {
        if (Socket.Get())
            Socket->Disconnect();
        SocketCtx.FreeBuffers();
        SocketCtx.CachedSession.Destroy();
    }

    ssize_t read(void *buffer, size_t buflen) {
        Y_VERIFY(Socket.Get(), "HttpsAgent: socket is picked out. Can't use until a valid socket is set");
        ssize_t ret = Socket->read(&SocketCtx, buffer, buflen);
        TTimer::OnAfterRecv();
        return ret;
    }

    int RequestGet(const char* url, const char * const*headers, int persistent = 1, bool head_request = false) {
        Y_VERIFY(Socket.Get(), "HttpsAgent: socket is picked out. Can't use until a valid socket is set");
        char message[MessageMax];
        ssize_t messlen = 0;
        if (PostData) {
            strcpy(message, "POST ");
            messlen = 5;
        } else if (head_request) {
            strcpy(message, "HEAD ");
            messlen = 5;
        } else {
            strcpy(message, "GET ");
            messlen = 4;
        }
#define _AppendMessage(mes) messlen += Min(MessageMax - messlen, \
(ssize_t)strlcpy(message + messlen, (mes), MessageMax - messlen))
        _AppendMessage(url);
        _AppendMessage(" HTTP/1.1\r\n");
        _AppendMessage(Hostheader);
        _AppendMessage("Connection: ");
        _AppendMessage(persistent? "Keep-Alive\r\n": "Close\r\n");
        while (headers && *headers)
            _AppendMessage(*headers++);
        if (AltFooter)
            _AppendMessage(AltFooter);
        else
            _AppendMessage(Footer);
        _AppendMessage("\r\n");
#undef _AppendMessage
        if (messlen >= MessageMax)
            return HTTP_HEADER_TOO_LARGE;

        if (!Persistent)
            Socket->Disconnect(&SocketCtx);
        Persistent = persistent;
        int connected = Socket->Good();
        SocketCtx.FreeBuffers();
        if (Scheme == THttpURL::SchemeHTTPS)
            SocketCtx.AllocBuffers();
        TTimer::OnBeforeSend();
        for (int attempt = !connected; attempt < 2; attempt++) {
            if (!Socket->Good() && Socket->Connect(&SocketCtx, Address, Timeout, Scheme == THttpURL::SchemeHTTPS, true)) {
                SocketCtx.FreeBuffers();
                return SocketCtx.SslError ? HTTP_SSL_ERROR : HTTP_CONNECT_FAILED;
            } else { // We successfully connected
                connected = true;
            }

            int sendOk = Socket->send(&SocketCtx, message, messlen);
            if (sendOk && PostData && PostDataLen)
                sendOk = Socket->send(&SocketCtx, PostData, PostDataLen);
            if (!sendOk) {
                int err = errno;
                Socket->Disconnect(&SocketCtx);
                errno = err;
                continue;
            }
            TTimer::OnAfterSend();

            if (!Socket->peek(&SocketCtx)) {
                int err = errno;
                Socket->Disconnect(&SocketCtx);
                if (err == EINTR) {
                    errno = err;
                    SocketCtx.FreeBuffers();
                    return HTTP_INTERRUPTED;
                }
                if(err == ETIMEDOUT) {
                    errno = err;
                    SocketCtx.FreeBuffers();
                    return HTTP_TIMEDOUT_WHILE_BYTES_RECEIVING;
                }
            } else {
                TTimer::OnBeforeRecv();
                if (!persistent)
                    Socket->shutdown();
                return 0;
            }
        }
        SocketCtx.FreeBuffers();
        return SocketCtx.SslError ? HTTP_SSL_ERROR : (connected ? HTTP_CONNECTION_LOST : HTTP_CONNECT_FAILED);
    }

    ui16 CertCheckErrors() const {
        return SocketCtx.CertErrors;
    }

protected:
    THolder<TSocket> Socket;
    typename TSocket::TSocketCtx SocketCtx;
    TNetworkAddress Address;
    int Scheme;
    int Persistent;
    TDuration Timeout;
    char *Hostheader, *Footer, *pHostBeg, *pHostEnd;
    const char *AltFooter; // alternative footer can be set by the caller
    const char *PostData;
    size_t PostDataLen;
    unsigned short HostheaderLen;
    static const ssize_t MessageMax = 4096;
};

} // namespace fetch
