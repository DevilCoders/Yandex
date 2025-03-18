#include "messenger.h"
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/thread/pool.h>
#include <util/generic/ymath.h>

Y_UNIT_TEST_SUITE(MediatorSuite) {
    class TSimpleMessage: public IMessage {
    private:
        TAtomic Counter = 0;

    public:
        void Prepare() override {
            Counter = 0;
        }

        virtual bool DoAdd() {
            AtomicIncrement(Counter);
            return true;
        }

        ui64 GetCounter() const {
            return AtomicGet(Counter);
        }
    };

    class TOneTimeProcessingSimpleMessage: public TSimpleMessage {
    public:
        bool DoAdd() override {
            TSimpleMessage::DoAdd();
            return false;
        }
    };

    class TSimpleProcessor: public IMessageProcessor {
    private:
        bool IsRegistered = false;

    public:
        TSimpleProcessor() {
            RegisterGlobalMessageProcessor(this);
            IsRegistered = true;
        }

        ~TSimpleProcessor() override {
            Y_VERIFY(IsRegistered);
            UnregisterGlobalMessageProcessor(this);
        }

        TString Name() const override {
            return "TestProcessor";
        }

        bool Process(IMessage* message) override {
            TSimpleMessage* messageSimple = dynamic_cast<TSimpleMessage*>(message);
            if (messageSimple) {
                return messageSimple->DoAdd();
            }
            return false;
        }
    };

    class TRecursiveMessage: public TSimpleMessage {
    private:
        ui32 CountRecursive = 0;
        TVector<TAtomicSharedPtr<TSimpleProcessor>> NewProcessors;

    public:
        TRecursiveMessage(const ui32 countRecursive)
            : CountRecursive(countRecursive)
        {
        }

        bool DoAdd() override {
            if (CountRecursive) {
                NewProcessors.push_back(new TSimpleProcessor);
                --CountRecursive;
            }
            TSimpleMessage::DoAdd();
            return true;
        }
    };

    Y_UNIT_TEST(SimpleCounter) {
        {
            TSimpleProcessor processor;
            UNIT_ASSERT_EQUAL(SendGlobalMessage<TSimpleMessage>().GetCounter(), 1);
            UNIT_ASSERT_EQUAL(SendGlobalMessage<TOneTimeProcessingSimpleMessage>().GetCounter(), 1);
            UNIT_ASSERT_EQUAL(SendGlobalMessage<TOneTimeProcessingSimpleMessage>().GetCounter(), 0);
            UNIT_ASSERT_EQUAL(SendGlobalMessage<TSimpleMessage>().GetCounter(), 1);
        }
        UNIT_ASSERT_EQUAL(SendGlobalMessage<TOneTimeProcessingSimpleMessage>().GetCounter(), 0);
        UNIT_ASSERT_EQUAL(SendGlobalMessage<TSimpleMessage>().GetCounter(), 0);
    }

    class TParallelProcessor: public IObjectInQueue {
    private:
    public:
        void Process(void* /*threadSpecificResource*/) override {
            THolder<TParallelProcessor> this_(this);
            TVector<TAtomicSharedPtr<TSimpleProcessor>> processors;
            for (i32 i = 0; i < 10; ++i) {
                processors.push_back(MakeAtomicShared<TSimpleProcessor>());
                SendGlobalMessage<TSimpleMessage>().GetCounter();
            }
        }
    };

    Y_UNIT_TEST(SimpleMultipleCounter) {
        TVector<TAtomicSharedPtr<TSimpleProcessor>> processors;
        for (ui64 i = 0; i < 100; ++i) {
            processors.push_back(new TSimpleProcessor);
            UNIT_ASSERT_EQUAL(SendGlobalMessage<TSimpleMessage>().GetCounter(), i + 1);
        }
    }

    Y_UNIT_TEST(MultipleCounter) {
        TSimpleThreadPool queue;
        queue.Start(16);
        for (ui32 i = 0; i < 100; ++i) {
            UNIT_ASSERT(queue.Add(new TParallelProcessor));
        }
        queue.Stop();
    }

    Y_UNIT_TEST(RecursiveNewCounter) {
        TRecursiveMessage message(3);
        SendGlobalMessage(message);
        UNIT_ASSERT_EQUAL(message.GetCounter(), 0);

        TSimpleProcessor processor;
        SendGlobalMessage(message);
        UNIT_ASSERT_EQUAL(message.GetCounter(), 4);
        SendGlobalMessage(message);
        UNIT_ASSERT_EQUAL(message.GetCounter(), 4);
    }

    class TParallelProcessorRecursive: public IObjectInQueue {
    private:
    public:
        void Process(void* /*threadSpecificResource*/) override {
            THolder<TParallelProcessorRecursive> this_(this);
            for (ui32 i = 0; i < 10; ++i) {
                UNIT_ASSERT(SendGlobalMessage<TRecursiveMessage>(3).GetCounter() >= 4);
            }
        }
    };

    Y_UNIT_TEST(RecursiveNewCounterMT) {
        TSimpleProcessor processor;
        TSimpleThreadPool queue;
        queue.Start(16);
        for (ui32 i = 0; i < 100; ++i) {
            UNIT_ASSERT(queue.Add(new TParallelProcessorRecursive));
        }
        queue.Stop();
    }
}
