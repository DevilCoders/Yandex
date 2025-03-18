#pragma once

#include "messenger.h"

#include <library/cpp/deprecated/atomic/atomic.h>
#include <library/cpp/logger/global/global.h>

#include <util/generic/map.h>
#include <util/system/rwlock.h>
#include <util/generic/list.h>
#include <util/generic/ptr.h>
#include <util/thread/pool.h>
#include <util/system/thread.h>
#include <util/system/mutex.h>
#include <util/system/event.h>

namespace NMessenger {
    class TMessageProcessorExecutor;

    class TMessengerImpl: public IMessenger {
    private:
        TAtomic ProcessorsCount = 0;
        typedef TList<TAtomicSharedPtr<TMessageProcessorExecutor>> TProcessorsList;
        TProcessorsList Processors;
        TMutex MutexModifyMessenger;
        TRWMutex SendMessageMutex;
        TRWMutex UnregisterMessengerMutex;

        TManualEvent StopEvent;
        THolder<TThread> ThreadCleaner;

        static void* RunCleaner(void* me) {
            ((TMessengerImpl*)(me))->Process(nullptr);
            return nullptr;
        }

        TThreadPool QueueMessagesAsync;

    public:
        virtual void Process(void* /*ThreadSpecificResource*/);

        void CheckEmpty() override;

        void Print();

        void CheckList() override;

        void CheckList(const TString& name) override;

        bool IsStoped;

    public:
        TMessengerImpl();

        virtual ~TMessengerImpl();

        void SendMessage(IMessage& message) override;

        class TSendMessageAsyncTask: public IObjectInQueue {
        private:
            IMessage::TPtr Message;
            TMessengerImpl* Messenger;

        public:
            TSendMessageAsyncTask(IMessage::TPtr message, TMessengerImpl* messenger)
                : Message(message)
                , Messenger(messenger)
            {
            }

            virtual void Process(void* /*threadSpecificResource*/) {
                THolder<TSendMessageAsyncTask> this_(this);
                Messenger->SendMessage(*Message);
            }
        };

        void SendMessageAsync(IMessage::TPtr message) override {
            CHECK_WITH_LOG(QueueMessagesAsync.Add(new TSendMessageAsyncTask(message, this)));
        }

        virtual void ClearProcessors();

        void RegisterMessageProcessor(IMessageProcessor* messageProcessor) override;

        void UnRegisterMessageProcessor(IMessageProcessor* messageProcessor) override;
    };
}

template <>
struct TSingletonTraits<NMessenger::TMessengerImpl> {
    static constexpr size_t Priority = 1000;
};
