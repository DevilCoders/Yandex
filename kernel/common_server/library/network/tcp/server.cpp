#include "server.h"

#include <library/cpp/logger/global/global.h>
#include <library/cpp/neh/asio/asio.h>
#include <library/cpp/neh/asio/executor.h>

#include <util/generic/buffer.h>
#include <util/memory/blob.h>
#include <util/system/mutex.h>

namespace {
    template <class T>
    const void* _Address(const T* p) {
        return static_cast<const void*>(p);
    }
}

namespace NUtil {
    class TTcpServer::TConnection: public TTcpServer::IConnection {
    public:
        TConnection(TTcpServer* server, TTcpServer::TSocketPtr socket)
            : Server(server)
            , Socket(socket)
            , Buffer(16000) // TODO
            , Canceled(0)
        {
            CHECK_WITH_LOG(Server);
            CHECK_WITH_LOG(Socket);
            RemoteHost = NAddr::PrintHostAndPort(*Socket->RemoteEndpoint().Addr());
        }

        virtual bool Alive() const override {
            return Canceled == 0;
        }

        virtual void Drop() override {
            Cancel();
        }

        virtual const TString& GetRemoteAddr() const override {
            return RemoteHost;
        }

        virtual void Read(TReadHandler&& rh) override {
            ReadAsync(std::move(rh));
        }

        virtual void Write(TBuffer&& buffer) override {
            WriteAsync(std::move(buffer));
        }

    private:
        void Cancel() {
            if (AtomicCas(&Canceled, 1, 0)) {
                DEBUG_LOG << RemoteHost << ": canceling" << Endl;
                Y_ASSERT(Socket);
                Socket->AsyncCancel();
            }
        }

        void ReadSync() {
            Y_ASSERT(Socket);
            Y_ASSERT(Buffer.Empty());
            auto guard = Guard(Lock);
            DEBUG_LOG << RemoteHost << ": reading" << Endl;
            while (true) {
                NAsio::TErrorCode ec;
                size_t req = Buffer.Avail();
                size_t read = Socket->ReadSome(Buffer.End(), req, ec);
                if (ec) {
                    ERROR_LOG << RemoteHost << ": " << "cannot sync read: " << ec.Value() << " " << ec.Text() << Endl;
                    Cancel();
                    return;
                }
                Y_ASSERT(read <= req);
                Buffer.Advance(read);
                if (read == req) {
                    Buffer.Reserve(2 * std::max<size_t>(Buffer.Size(), 1));
                } else {
                    return;
                }
            }
        }

        void ReadAsync(TReadHandler&& rh) {
            auto this_ = TIntrusivePtr<TConnection>(this);
            auto handler = [this_, rh_ = std::move(rh)] (const NAsio::TErrorCode& ec, NAsio::IHandlingContext& /*ctx*/) mutable {
                if (!ec) {
                    auto& buffer = this_->Buffer;
                    Y_ASSERT(buffer.Empty());
                    this_->ReadSync();
                    if (!buffer.Empty()) {
                        rh_(buffer);
                        buffer.Clear();
                    } else {
                        ERROR_LOG << this_->GetRemoteAddr() << ": read 0 bytes" << Endl;
                        this_->Cancel();
                    }
                } else {
                    ERROR_LOG << this_->GetRemoteAddr() << ": " << "cannot async poll: " << ec.Value() << " " << ec.Text() << Endl;
                    this_->Cancel();
                }
                if (!AtomicGet(this_->Canceled)) {
                    this_->Read(std::move(rh_));
                }
            };
            DEBUG_LOG << RemoteHost << ": " << "setting AsyncPollRead handler" << Endl;
            Y_ASSERT(Server);
            Y_ASSERT(Socket);
            Socket->AsyncPollRead(std::move(handler), Server->GetOptions().ReadTimeout);
        }

        void WriteSync(const char* data, size_t size) {
            Y_ASSERT(Socket);
            auto guard = Guard(Lock);
            size_t written = 0;
            while (written < size) {
                DEBUG_LOG << RemoteHost << ": " << "writing " << _Address(data + written) << " " << size - written << " bytes";
                NAsio::TErrorCode ec;
                written += Socket->WriteSome(data + written, size - written, ec);
                if (ec) {
                    ERROR_LOG << RemoteHost << ": " << "cannot sync write " << size - written << " bytes: " << ec.Value() << " " << ec.Text() << Endl;
                    Cancel();
                    return;
                }
            }
            Y_ASSERT(written <= size);
        }

        void WriteAsync(TBuffer&& buffer) {
            auto blob = TBlob::FromBuffer(buffer);
            auto this_ = TIntrusivePtr<TConnection>(this);
            auto handler = [this_, blob] (const NAsio::TErrorCode& ec, size_t written, NAsio::IHandlingContext& ctx) {
                Y_UNUSED(ctx);
                if (!ec) {
                    Y_ASSERT(written <= blob.Size());
                    if (written < blob.Size()) {
                        this_->WriteSync(blob.AsCharPtr() + written, blob.Size() - written);
                    }
                    DEBUG_LOG << this_->GetRemoteAddr() << ": " << "successfully written " << blob.Size() << " bytes" << Endl;
                } else {
                    ERROR_LOG << this_->GetRemoteAddr() << ": " << "cannot async write " << blob.Size() << " bytes: " << ec.Value() << " " << ec.Text() << Endl;
                    this_->Cancel();
                }
            };
            Y_ASSERT(Server);
            Y_ASSERT(Socket);
            Y_ASSERT(!blob.Empty());
            Socket->AsyncWrite(blob.Data(), blob.Size(), std::move(handler), Server->GetOptions().WriteTimeout);
            DEBUG_LOG << RemoteHost << ": " << "scheduled writing " << _Address(blob.Data()) << " " << blob.Size() << " bytes" << Endl;
        }

    private:
        TTcpServer* Server;
        TTcpServer::TSocketPtr Socket;
        TString RemoteHost;

        TMutex Lock;
        TBuffer Buffer;
        TAtomic Canceled;
    };
}

NUtil::TTcpServer::TConnectionOutput::TConnectionOutput(TConnectionPtr connection)
    : Connection(connection)
    , Sent(0)
{
    CHECK_WITH_LOG(Connection);
}

void NUtil::TTcpServer::TConnectionOutput::DoWrite(const void* buf, size_t len) {
    Y_ASSERT(len);
    Y_ASSERT(buf);
    Output.Write(buf, len);
}

void NUtil::TTcpServer::TConnectionOutput::DoFlush() {
    Output.Flush();
    if (!Output.Buffer().Empty()) {
        Y_ASSERT(Connection);
        const size_t size = Output.Buffer().Size();
        DEBUG_LOG << Connection->GetRemoteAddr() << ": flushing " << size << " bytes" << Endl;
        Connection->Write(std::move(Output.Buffer()));
        Sent += size;
    }
}

NUtil::TTcpServer::TTcpServer(ui16 port, const TOptions& options, TCallbackPtr callback)
    : Port(port)
    , Options(options)
    , Callback(callback)
{
    CHECK_WITH_LOG(Callback);
}

NUtil::TTcpServer::~TTcpServer() {
    if (AcceptExecutor && ProcessExecutors) {
        Stop();
    }
}

void NUtil::TTcpServer::Start() {
    CHECK_WITH_LOG(!Acceptors);
    CHECK_WITH_LOG(!AcceptExecutor);
    AcceptExecutor = MakeHolder<NAsio::TIOServiceExecutor>();

    CHECK_WITH_LOG(!ProcessExecutors);
    ProcessExecutors = MakeHolder<NAsio::TExecutorsPool>(Options.Threads);

    TNetworkAddress addr(Port);
    for (TNetworkAddress::TIterator it = addr.Begin(); it != addr.End(); ++it) {
        TEndpoint ep(new NAddr::TAddrInfo(&*it));
        auto acceptor = MakeAtomicShared<NAsio::TTcpAcceptor>(AcceptExecutor->GetIOService());
        acceptor->Bind(ep);
        acceptor->Listen(Options.ListenBacklog);
        StartAccept(acceptor);
        Acceptors.push_back(acceptor);
    }
}

void NUtil::TTcpServer::Stop() {
    CHECK_WITH_LOG(AcceptExecutor);
    AcceptExecutor->SyncShutdown();
    AcceptExecutor.Destroy();

    Acceptors.clear();

    CHECK_WITH_LOG(ProcessExecutors);
    ProcessExecutors->SyncShutdown();
    ProcessExecutors.Destroy();
}

void NUtil::TTcpServer::StartAccept(TAcceptorPtr acceptor) {
    using namespace std::placeholders;
    CHECK_WITH_LOG(ProcessExecutors);
    CHECK_WITH_LOG(AcceptExecutor);
    auto socket = MakeAtomicShared<NAsio::TTcpSocket>(ProcessExecutors->Size() ? ProcessExecutors->GetExecutor().GetIOService() : AcceptExecutor->GetIOService());
    acceptor->AsyncAccept(*socket, std::bind(&NUtil::TTcpServer::OnAccept, this, acceptor, socket, _1, _2));
}

void NUtil::TTcpServer::OnAccept(TAcceptorPtr acceptor, TSocketPtr socket, const NAsio::TErrorCode& ec, NAsio::IHandlingContext& /*ctx*/) {
    if (!ec) {
        SetNoDelay(socket->Native(), true);
        SetNonBlock(socket->Native());
        auto connection = MakeIntrusive<TConnection>(this, socket);
        DEBUG_LOG << "created connection" << Endl;
        CHECK_WITH_LOG(Callback);
        Callback->OnConnection(connection);
    } else {
        if (ec.Value() == ECANCELED) {
            return;
        } else {
            ERROR_LOG << ec.Value() << " " << ec.Text() << Endl;
        }
    }
    StartAccept(acceptor);
}
