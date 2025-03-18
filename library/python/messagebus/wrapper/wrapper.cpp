#include "wrapper.h"

#include <util/datetime/cputimer.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>

#include <queue>

namespace NMessageBusWrapper {
    namespace {
        TAutoPtr<NBus::TBusMessage> CreateBusMessage(
            TProtocol* protocol, const TString& messageName,
            const TString& messageBytes) {
            TAutoPtr<NBus::TBusBufferBase> message =
                protocol->CreateMessageByProtoName(messageName);
            if (!message) {
                throw yexception() << NBus::GetMessageStatus(NBus::MESSAGE_UNKNOWN);
            }
            Y_PROTOBUF_SUPPRESS_NODISCARD message->GetRecord()->ParseFromString(messageBytes);
            return message.Release();
        }

    }

    class TSyncServerIteratorInternal: public NBus::IBusServerHandler,
                                        public TSimpleRefCount<TSyncServerIteratorInternal> {
    public:
        TSyncServerIteratorInternal(NBus::TBusMessageQueuePtr queue)
            : Queue(queue)
        {
        }

        ~TSyncServerIteratorInternal() override {
            Session->Shutdown();
        }

        void SetSession(NBus::TBusServerSessionPtr session) {
            Session = session;
        }

        bool Next(TMessage* msg);

        virtual void OnReply(TAutoPtr<NBus::TBusMessage> pMessage, TAutoPtr<NBus::TBusMessage> pReply);
        void OnError(TAutoPtr<NBus::TBusMessage> pMessage, NBus::EMessageStatus status) override;
        void OnMessage(NBus::TOnMessageContext& pMessage) override;
        void OnSent(TAutoPtr<NBus::TBusMessage> pMessage) override;
        virtual void OnMessageSent(NBus::TBusMessage* pMessage);

    private:
        NBus::TBusMessageQueuePtr Queue;
        NBus::TBusServerSessionPtr Session;

        std::queue<TMessage> ReceivedMessages;
        TMutex Lock;
        TCondVar Cond;
    };

    ///////////////////////////
    // TMessageBus

    TMessageBus::TMessageBus()
        : Queue(NBus::CreateMessageQueue())
    {
    }

    TMessageBus::~TMessageBus() {
    }

    void TMessageBus::Register(
        const TProtocol& proto, const TString& hostName, int port,
        NBus::TBusKey begin, NBus::TBusKey end, bool useIpv4 /* = true */) {
        NBus::EIpVersion ipVersion = useIpv4 ? NBus::EIP_VERSION_4 : NBus::EIP_VERSION_6;
        Queue->GetLocator()->Register(proto.GetService().data(), hostName.data(), port, begin, end, ipVersion);
    }

    void TMessageBus::Register(
        const TProtocol& proto, const TString& hostName, int port,
        bool useIpv4 /* = true */) {
        using namespace NBus;
        NBus::EIpVersion ipVersion = useIpv4 ? EIP_VERSION_4 : EIP_VERSION_6;
        Queue->GetLocator()->Register(proto.GetService().data(), hostName.data(), port, YBUS_KEYMIN, YBUS_KEYMAX, ipVersion);
    }

    TSyncSession TMessageBus::CreateSyncSource(TProtocol proto, bool needReply, int timeout) {
        NBus::TBusSessionConfig config;
        config.TotalTimeout = timeout;
        config.SendTimeout = timeout;
        return TSyncSession(
            Queue->CreateSyncSource(proto.GetProto(), config, needReply),
            proto, Queue);
    }

    TSyncServer TMessageBus::CreateSyncServer(TProtocol protocol) {
        TSyncServerIteratorInternal* iterator = new TSyncServerIteratorInternal(Queue);
        NBus::TBusServerSessionPtr session =
            NBus::TBusServerSession::Create(protocol.GetProto(), iterator, NBus::TBusSessionConfig(), Queue);
        iterator->SetSession(session);
        return TSyncServer(session, protocol, iterator, Queue);
    }

    ///////////////////////
    // TSyncSession

    TSyncSession::TSyncSession(
        NBus::TBusSyncClientSessionPtr session,
        const TProtocol& protocol,
        NBus::TBusMessageQueuePtr queue)
        : Queue(queue)
        , Protocol(protocol)
        , Session(session)
    {
    }

    ////////////////////////////////////////////////////////////
    /// \brief Selects address for host and port.

    /// Returns first IPv4 address, if available.
    /// Returns first IPv6 address, if available and no IPv4 address.
    /// Returns NULL otherwise.
    TAutoPtr<NBus::TNetAddr> GetDestinationAddr(const char* hostName, int port) {
        if (!hostName || !*hostName) {
            return nullptr;
        }
        TAutoPtr<NBus::TNetAddr> ret;
        TNetworkAddress netAddress(hostName, port);
        for (TNetworkAddress::TIterator ai = netAddress.Begin(); ai != netAddress.End(); ++ai) {
            bool ipv4Compatible = IsFamilyAllowed(ai->ai_addr->sa_family, /*ipVersion=*/NBus::EIP_VERSION_4);
            if (!ipv4Compatible && !!ret) {
                continue;
            }
            ret = new NBus::TNetAddr(netAddress, &*ai);
            if (ipv4Compatible) {
                return ret;
            }
        }
        return ret;
    }

    /// @throws yexception
    void TSyncSession::SendBytes(
        const TString& messageName,
        const TString& bytes,
        const TString& destinationHost,
        int destinationPort,
        bool needReply,
        TString* outMessageName,
        TString* outMessageBytes) {
        TAutoPtr<NBus::TBusBufferBase> message(Protocol.CreateMessageByProtoName(messageName));
        if (!message) {
            throw yexception() << NBus::GetMessageStatus(NBus::MESSAGE_UNKNOWN);
        }
        Y_PROTOBUF_SUPPRESS_NODISCARD message->GetRecord()->ParseFromString(bytes);
        NBus::EMessageStatus ret;
        THolder<NBus::TNetAddr> dest(GetDestinationAddr(destinationHost.data(), destinationPort));
        TAutoPtr<NBus::TBusMessage> reply = Session->SendSyncMessage(
            message.Get(), ret, dest.Get());
        if (ret != NBus::MESSAGE_OK) {
            throw yexception() << NBus::GetMessageStatus(ret);
        }
        NBus::TBusBufferBase* replyProto = static_cast<NBus::TBusBufferBase*>(reply.Get());
        if ((bool)reply.Get() != needReply) {
            throw yexception() << "Expected to have " << (needReply ? "" : "no ") << "reply";
        }
        if (needReply) {
            Y_VERIFY(replyProto, "Expected message to be TBusBufferBase");

            Y_PROTOBUF_SUPPRESS_NODISCARD replyProto->GetRecord()->SerializeToString(outMessageBytes);
            *outMessageName = replyProto->GetRecord()->GetDescriptor()->full_name();
        }
    }

    ///////////////////////////
    // TMessage

    TMessage::TMessage()
        : Queue(nullptr)
        , Session(nullptr)
        , Iterator(nullptr)
        , OnMessageContext(nullptr)
    {
    }
    TMessage::TMessage(
        TAutoPtr<NBus::TOnMessageContext> msg,
        NBus::TBusMessageQueuePtr queue,
        NBus::TBusServerSessionPtr session,
        TSyncServerIteratorInternal* iterator)
        : Queue(queue)
        , Session(session)
        , Iterator(iterator)
        , OnMessageContext(msg)
    {
    }

    TMessage::TMessage(const TMessage& message)
        : Queue(message.Queue)
        , Session(message.Session)
        , Iterator(message.Iterator)
        , OnMessageContext(message.OnMessageContext)
    {
    }

    TMessage::~TMessage() {
        if (OnMessageContext.RefCount() == 1 && OnMessageContext->IsInWork()) {
            OnMessageContext->ForgetRequest();
        }
    }

    TString TMessage::GetMessageName() const {
        return GetMBusMessage()->GetRecord()->GetDescriptor()->full_name();
    }

    TString TMessage::GetPayload() const {
        TString res;
        Y_PROTOBUF_SUPPRESS_NODISCARD GetMBusMessage()->GetRecord()->SerializeToString(&res);
        return res;
    }

    NBus::TOnMessageContext* TMessage::GetOnMessageContext() const {
        return OnMessageContext.Get();
    }

    NBus::TBusBufferBase* TMessage::GetMBusMessage() const {
        return VerifyDynamicCast<NBus::TBusBufferBase*>(OnMessageContext->GetMessage());
    }

    void TMessage::CheckMessage() {
        NBus::TBusBufferBase* bufMessage = dynamic_cast<NBus::TBusBufferBase*>(GetMBusMessage());
        if (!bufMessage) {
            throw yexception() << "Unexpected message";
        }
    }

    ///////////////////////////
    // TSyncServerIterator

    TSyncServerIterator::TSyncServerIterator(const TSyncServerIterator& iter)
        : Iterator(iter.Iterator)
    {
    }

    TSyncServerIterator::TSyncServerIterator(TSyncServerIteratorInternal* iter)
        : Iterator(iter)
    {
    }

    TSyncServerIterator::~TSyncServerIterator() {
    }

    /// @throws yexception
    TMessage TSyncServerIterator::Next(bool* success) {
        TMessage mess;
        *success = Iterator->Next(&mess);
        return mess;
    }

    ///////////////////////////
    // TSyncServerIteratorInternal

    bool TSyncServerIteratorInternal::Next(TMessage* message) {
        TGuard<TMutex> g(Lock);
        const TDuration maxDuration = TDuration::MilliSeconds(100);
        TSimpleTimer timer;
        while (ReceivedMessages.empty() && timer.Get() < maxDuration) {
            Cond.WaitT(Lock, maxDuration);
        }

        if (ReceivedMessages.empty()) {
            return false;
        }

        *message = ReceivedMessages.front();
        ReceivedMessages.pop();
        message->CheckMessage();
        return true;
    }

    void TSyncServerIteratorInternal::OnReply(
        TAutoPtr<NBus::TBusMessage> pMessage,
        TAutoPtr<NBus::TBusMessage> pReply) {
        Y_UNUSED(pMessage);
        Y_UNUSED(pReply);
        Y_FAIL("Should never be called by destination session");
    }

    void TSyncServerIteratorInternal::OnError(
        TAutoPtr<NBus::TBusMessage> pMessage,
        NBus::EMessageStatus status) {
        // We failed to deliver message forget it and let TAutoPtr remove it.
        Y_UNUSED(status);
        Y_UNUSED(pMessage);
    }

    void TSyncServerIteratorInternal::OnMessage(NBus::TOnMessageContext& pMessage) {
        // New message has arrived.
        TGuard<TMutex> g(Lock);
        TAutoPtr<NBus::TOnMessageContext> tmp = new NBus::TOnMessageContext;
        pMessage.Swap(*tmp);

        ReceivedMessages.push(TMessage(tmp, Queue, Session, this));
        Cond.Signal();
    }

    void TSyncServerIteratorInternal::OnSent(TAutoPtr<NBus::TBusMessage> pMessage) {
        // Just let TAutoPtr delete the message.
        Y_UNUSED(pMessage);
    }

    void TSyncServerIteratorInternal::OnMessageSent(NBus::TBusMessage* pMessage) {
        Y_UNUSED(pMessage);
        Y_FAIL("Must never be called by destination session");
    }

    ///////////////////////////
    // TSyncServer

    TSyncServer::TSyncServer(const TSyncServer& server)
        : Queue(server.Queue)
        , Protocol(server.Protocol)
        , Session(server.Session)
        , Iterator(server.Iterator)
    {
    }

    TSyncServer::TSyncServer(
        NBus::TBusServerSessionPtr session,
        const TProtocol& protocol,
        TSyncServerIteratorInternal* iterator,
        NBus::TBusMessageQueuePtr queue)
        : Queue(queue)
        , Protocol(protocol)
        , Session(session)
        , Iterator(iterator)
    {
    }

    TSyncServer::~TSyncServer() {
    }

    TSyncServerIterator TSyncServer::IncomingMessages() {
        return TSyncServerIterator(Iterator.Get());
    }

    /// @throws yexception
    void TSyncServer::SendReply(TMessage message, const TString& messageName,
                                const TString& bytes) {
        TAutoPtr<NBus::TBusMessage> reply = CreateBusMessage(
            &Protocol, messageName, bytes);

        NBus::EMessageStatus status = message.GetOnMessageContext()->SendReplyMove(reply.Release());
        if (status != NBus::MESSAGE_OK) {
            throw yexception() << NBus::GetMessageStatus(status);
        }
    }

    /// @throws yexception
    void TSyncServer::Discard(TMessage message) {
        message.GetOnMessageContext()->ForgetRequest();
    }

}
