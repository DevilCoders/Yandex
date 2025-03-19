#pragma once

#include <util/datetime/base.h>
#include <util/generic/fwd.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/buffer.h>

class TBuffer;

namespace NAsio {
    class IHandlingContext;
    class TErrorCode;
    class TExecutorsPool;
    class TIOServiceExecutor;
    class TTcpAcceptor;
    class TTcpSocket;
}

namespace NUtil {
    class TTcpServer {
    public:
        class IConnection: public TAtomicRefCount<IConnection> {
        public:
            using TReadHandler = std::function<void(const TBuffer&)>;
        public:
            virtual ~IConnection() = default;

            virtual bool Alive() const = 0;
            virtual void Drop() = 0;
            virtual const TString& GetRemoteAddr() const = 0;

            virtual void Read(TReadHandler&& rh) = 0;
            virtual void Write(TBuffer&& buffer) = 0;
        };
        using TConnectionPtr = TIntrusivePtr<IConnection>;

        class TConnectionOutput: public IOutputStream {
        public:
            TConnectionOutput(TConnectionPtr connection);

            size_t GetBytesSent() const {
                return Sent;
            }

        protected:
            virtual void DoWrite(const void* buf, size_t len) override;
            virtual void DoFlush() override;

        private:
            TConnectionPtr Connection;
            TBufferOutput Output;

            size_t Sent;
        };

        class ICallback {
        public:
            virtual ~ICallback() = default;

            virtual void OnConnection(TConnectionPtr connection) = 0;
        };
        using TCallbackPtr = TAtomicSharedPtr<ICallback>;

        struct TOptions {
            int ListenBacklog = 100;
            size_t Threads = 16;
            TDuration ReadTimeout = TDuration::Seconds(30);
            TDuration WriteTimeout = TDuration::Seconds(10);
        };

    public:
        TTcpServer(ui16 port, const TOptions& options, TCallbackPtr callback);
        ~TTcpServer();

        const TOptions& GetOptions() const {
            return Options;
        }
        ui16 GetPort() const {
            return Port;
        }

        void Start();
        void Stop();

    private:
        class TConnection;
        using TAcceptorPtr = TAtomicSharedPtr<NAsio::TTcpAcceptor>;
        using TSocketPtr = TAtomicSharedPtr<NAsio::TTcpSocket>;

    private:
        void StartAccept(TAcceptorPtr acceptor);
        void OnAccept(TAcceptorPtr acceptor, TSocketPtr socket, const NAsio::TErrorCode& ec, NAsio::IHandlingContext& ctx);

    private:
        const ui16 Port;
        const TOptions Options;
        TCallbackPtr Callback;

        TVector<TAcceptorPtr> Acceptors;
        THolder<NAsio::TIOServiceExecutor> AcceptExecutor;
        THolder<NAsio::TExecutorsPool> ProcessExecutors;
    };
}
