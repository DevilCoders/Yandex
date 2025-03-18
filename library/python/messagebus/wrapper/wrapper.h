#pragma once

#include <library/cpp/messagebus/ybus.h>
#include <library/cpp/messagebus/protobuf/ybusbuf.h>

#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <util/system/sanitizers.h>

namespace NMessageBusWrapper {
    const int DEFAULT_TIMEOUT_MS = NSan::PlainOrUnderSanitizer(60000, 180000);

    class TAddress;
    class TMessageBus;
    class TProtocol;
    class TSyncSession;
    class TSyncServer;
    class TSyncServerIteratorInternal;

    class TMessageBus {
    public:
        TMessageBus();
        ~TMessageBus();
        void Register(const TProtocol& proto, const TString& hostName,
                      int port, NBus::TBusKey begin, NBus::TBusKey end,
                      bool useIpv4 = true);
        void Register(const TProtocol& proto, const TString& hostName,
                      int port, bool useIpv4 = true);
        TSyncSession CreateSyncSource(TProtocol proto, bool needReply = true, int timeout = DEFAULT_TIMEOUT_MS);
        TSyncServer CreateSyncServer(TProtocol proto);

    private:
        NBus::TBusMessageQueuePtr Queue;
    };

    class TProtocol {
    public:
        TProtocol()
            : Proto()
        {
        }

        TProtocol(NBus::TBusBufferProtocol* proto)
            : Proto(proto)
        {
        }

        TString GetService() const {
            return Proto->GetService();
        }

        NBus::TBusBufferProtocol* GetProto() {
            return Proto.Get();
        }

        NBus::TBusBufferBase* CreateMessageByProtoName(const TString& protoName) const {
            for (auto i : Proto->GetTypes()) {
                TString curName = i->GetRecord()->GetDescriptor()->full_name();
                if (curName == protoName) {
                    return i->New();
                }
            }
            return nullptr;
        }

    private:
        TSimpleSharedPtr<NBus::TBusBufferProtocol> Proto;
    };

    class TSyncSession {
        friend class TMessageBus;

    public:
        /// @throws yexception
        void SendBytes(
            const TString& messageName,
            const TString& bytes,
            const TString& destinationHost,
            int destinationPort,
            bool needReply,
            TString* replyName,
            TString* replyBytes);

    private:
        TSyncSession(NBus::TBusSyncClientSessionPtr session,
                     const TProtocol& protocol,
                     NBus::TBusMessageQueuePtr queue);

    private:
        // We keep a pointer to MessageQueue here for the case when
        // TMessageBus object is deleted and TSyncSession object still exists.
        NBus::TBusMessageQueuePtr Queue;

        TProtocol Protocol;
        NBus::TBusSyncClientSessionPtr Session;
    };

    class TMessage {
    public:
        TMessage(const TMessage& message);
        ~TMessage();

        TString GetMessageName() const;
        TString GetPayload() const;

        NBus::TBusBufferBase* GetMBusMessage() const;
        NBus::TOnMessageContext* GetOnMessageContext() const;

        void CheckMessage();

    private:
        TMessage();
        TMessage(TAutoPtr<NBus::TOnMessageContext> msg,
                 NBus::TBusMessageQueuePtr queue,
                 NBus::TBusServerSessionPtr session,
                 TSyncServerIteratorInternal* iterator);

    private:
        // that stuff must not be deleted before message is deleted
        NBus::TBusMessageQueuePtr Queue;
        NBus::TBusServerSessionPtr Session;
        TIntrusivePtr<TSyncServerIteratorInternal> Iterator;

        // TBusMessage is refcounted, but it's incompatible with
        // TIntrusivePtr (it's Ref()ed in constructor), so we have
        // to use TSharedPtr.
        TSimpleSharedPtr<NBus::TOnMessageContext> OnMessageContext;

    private:
        friend class TSyncServerIterator;
        friend class TSyncServerIteratorInternal;
    };

    class TSyncServerIterator {
        friend class TSyncServer;

    public:
        TSyncServerIterator(const TSyncServerIterator& iter);
        ~TSyncServerIterator();

        /// Waits for the next message and returns it.
        /// If there is no message for 100 millisecs, returns empty message.
        ///
        /// We want to return control to Python every 100 milliseconds
        /// so Python can handle CTRL-C signal if it wants.
        ///
        /// Such strange interface of a method is caused by SWIG,
        /// with such interface it's easier to write SWIG file.
        ///
        /// @throws yexception
        TMessage Next(bool* success);

    private:
        TSyncServerIterator(TSyncServerIteratorInternal* iter);

    private:
        TIntrusivePtr<TSyncServerIteratorInternal> Iterator;
    };

    // TSyncServer is assumed to be used in the following way:
    // one gets TSyncServerIterator and starts to retrieve messages using it.
    //
    // Each message must be either replied using TSyncServer::SendReply or
    // discarded using TSyncServer::Discard,
    // otherwise there will be a leak that will make server stuck.
    class TSyncServer {
        friend class TMessageBus;

    public:
        TSyncServer(const TSyncServer& server);
        ~TSyncServer();

        TSyncServerIterator IncomingMessages();

        /// @throws yexception
        void SendReply(
            TMessage message,
            const TString& messageName,
            const TString& bytes);
        /// @throws yexception
        void Discard(TMessage message);

    private:
        TSyncServer(NBus::TBusServerSessionPtr session, const TProtocol& protocol,
                    TSyncServerIteratorInternal* iterator,
                    NBus::TBusMessageQueuePtr queue);

    private:
        // We keep a pointer to MessageQueue here for the case when
        // TMessageBus object is deleted and TSyncSession object still exists.
        NBus::TBusMessageQueuePtr Queue;

        TProtocol Protocol;
        NBus::TBusServerSessionPtr Session;
        TIntrusivePtr<TSyncServerIteratorInternal> Iterator;
    };

}
