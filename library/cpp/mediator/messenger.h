#pragma once

#include <util/generic/string.h>
#include <util/generic/singleton.h>
#include <util/generic/ptr.h>

namespace NMessenger {
    class IMessage {
    private:
        bool MessageIsProcessed = false;

    public:
        using TPtr = TAtomicSharedPtr<IMessage>;

    public:
        template <class T>
        T* As() {
            return dynamic_cast<T*>(this);
        }

        virtual ~IMessage(){};
        virtual void Prepare() {
        }
        void MessageIsProcessedSignal() {
            MessageIsProcessed = true;
        }
        bool GetMessageIsProcessed() const {
            return MessageIsProcessed;
        }
    };

    class IMessageProcessor {
    public:
        virtual ~IMessageProcessor() {
        }
        //! result must be true for message type applicable for this processor. false otherwise;
        virtual bool Process(IMessage* message) = 0;
        virtual TString Name() const = 0;
    };

    class IMessenger {
    public:
        virtual ~IMessenger() {
        }
        virtual void SendMessage(IMessage& message) = 0;
        virtual void SendMessageAsync(IMessage::TPtr message) = 0;
        virtual void RegisterMessageProcessor(IMessageProcessor* messageProcessor) = 0;
        virtual void UnRegisterMessageProcessor(IMessageProcessor* messageProcessor) = 0;
        virtual void CheckEmpty() = 0;
        virtual void CheckList() = 0;
        virtual void CheckList(const TString& name) = 0;
        static IMessenger* BuildMessenger();
    };

    class TGlobalMediator {
    public:
        static void SendMessage(IMessage& message);
        static void SendMessageAsync(IMessage::TPtr message);
        static void RegisterMessageProcessor(IMessageProcessor* messageProcessor);
        static void UnRegisterMessageProcessor(IMessageProcessor* messageProcessor);
        static void CheckEmpty();
        static void CheckList();
        static void CheckList(const TString& name);
    };

}

// compatibility typedefs
using IMessage = NMessenger::IMessage;
using IMessageProcessor = NMessenger::IMessageProcessor;
using IMessenger = NMessenger::IMessenger;

bool SendGlobalMessage(IMessage& message);
void SendGlobalMessageAsync(IMessage::TPtr message);
void RegisterGlobalMessageProcessor(NMessenger::IMessageProcessor* processor);
void UnregisterGlobalMessageProcessor(NMessenger::IMessageProcessor* processor);

Y_FORCE_INLINE bool SendGlobalDebugMessage(IMessage& message) {
#ifndef NDEBUG
    return SendGlobalMessage(message);
#else
    Y_UNUSED(message);
    return false;
#endif
}

template <class T>
Y_FORCE_INLINE T SendGlobalMessage() {
    T message;
    SendGlobalMessage(message);
    return message;
}

template <class T, class... Args>
Y_FORCE_INLINE T SendGlobalMessage(Args... args) {
    T message(args...);
    SendGlobalMessage(message);
    return message;
}

template <class T>
Y_FORCE_INLINE void SendGlobalMessageAsync() {
    SendGlobalMessageAsync(new T());
}

template <class T, class... Args>
Y_FORCE_INLINE void SendGlobalMessageAsync(Args... args) {
    SendGlobalMessageAsync(new T(args...));
}

template <class T>
Y_FORCE_INLINE T SendGlobalDebugMessage() {
    T message;
    SendGlobalDebugMessage(message);
    return message;
}

template <class T, class... Args>
Y_FORCE_INLINE T SendGlobalDebugMessage(Args... args) {
    T message(args...);
    SendGlobalDebugMessage(message);
    return message;
}
