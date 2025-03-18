#include "messenger_impl.h"

#include <util/system/mutex.h>
#include <util/system/guard.h>
#include <util/system/thread.h>

#include <library/cpp/balloc/optional/operators.h>

#include <typeindex>

namespace NMessenger {
    class TMessageProcessorExecutor {
    private:
        TAtomic Unused = 0;
        IMessageProcessor* Processor;
        TMap<std::type_index, bool> MarkersMessagesTypes;
        TRWMutex Mutex;
        TRWMutex MutexForUnused;
        TString Name_;

    public:
        bool IsContain(IMessageProcessor* processor) {
            return processor == Processor;
        }

        void MarkAsUnused() {
            TWriteGuard wg(MutexForUnused);
            AtomicSet(Unused, 1);
        }

        bool GetIsUnused() const {
            return AtomicGet(Unused);
        }

        TMessageProcessorExecutor(IMessageProcessor* processor)
            : Processor(processor)
        {
            Name_ = Processor->Name();
            Unused = false;
        }

        ~TMessageProcessorExecutor() {
            VERIFY_WITH_LOG(Unused, "Incorrect messenger usage");
        }

        TString Name() const {
            return Name_;
        }

        TString Info() const {
            return "Name : " + Name_ + ". Address " + ToString((const void*)Processor) + ". Unused : " + ToString(Unused);
        }

        void Execute(IMessage* message) {
            TReadGuard rgUnusedChange(MutexForUnused);
            if (AtomicGet(Unused))
                return;
            std::type_index typeIndex = std::type_index(typeid(*message));
            bool store = false;
            {
                TReadGuard rg(Mutex);
                auto it = MarkersMessagesTypes.find(typeIndex);
                store = (it == MarkersMessagesTypes.end());
                if (!store && !it->second) {
                    return;
                }
            }
            bool used = Processor->Process(message);
            if (store) {
                TWriteGuard g(Mutex);
                MarkersMessagesTypes[typeIndex] = used;
            }
            if (used) {
                message->MessageIsProcessedSignal();
            }
        }
    };

    void TMessengerImpl::Process(void* /*ThreadSpecificResource*/) {
        ThreadDisableBalloc();
        while (!StopEvent.WaitT(TDuration::Seconds(300))) {
            ClearProcessors();
        }
        ClearProcessors();
    }

    void TMessengerImpl::CheckEmpty() {
        CheckList();
        for (TProcessorsList::iterator i = Processors.begin(), e = Processors.end(); i != e; ++i) {
            if (!(*i)->GetIsUnused())
                FAIL_LOG("Messenger is not empty");
        }
    }

    void TMessengerImpl::Print() {
        for (TProcessorsList::iterator i = Processors.begin(), e = Processors.end(); i != e; ++i) {
            DEBUG_LOG << (*i)->Info() << Endl;
        }
    }

    void TMessengerImpl::CheckList(const TString& name) {
        for (TProcessorsList::iterator i = Processors.begin(), e = Processors.end(); i != e; ++i) {
            if (!(*i)->GetIsUnused())
                VERIFY_WITH_LOG((*i)->Name() != name, "CheckList failed for %s", name.data());
        }
    }

    void TMessengerImpl::CheckList() {
        for (TProcessorsList::iterator i = Processors.begin(), e = Processors.end(); i != e; ++i) {
            if (!(*i)->GetIsUnused())
                ERROR_LOG << (*i)->Name() << Endl;
        }
    }

    TMessengerImpl::TMessengerImpl() {
        INFO_LOG << "Messenger constructing..." << Endl;
        QueueMessagesAsync.Start(8);
        ThreadCleaner.Reset(new TThread(RunCleaner, this));
        ThreadCleaner->Start();
        INFO_LOG << "Messenger constructed...OK" << Endl;
    }

    TMessengerImpl::~TMessengerImpl() {
        INFO_LOG << "Messenger destroying..." << Endl;
        StopEvent.Signal();
        QueueMessagesAsync.Stop();
        ThreadCleaner->Join();
        CheckList();
        for (auto&& i : Processors) {
            ERROR_LOG << i->Name() << Endl;
        }
        CHECK_WITH_LOG(!Processors.size()) << "Incorrect messenger destroy context: processors list is not empty: " << Processors.size();
        INFO_LOG << "Messenger destroyed" << Endl;
    }

    void TMessengerImpl::SendMessage(IMessage& message) {
        TReadGuard rgSendMessage(SendMessageMutex);
        message.Prepare();
        TProcessorsList::iterator currentPosition = Processors.begin();
        i64 currentPositionIdx = 0;
        i64 finishPositionIdx = AtomicGet(ProcessorsCount) - 1;
        while (currentPositionIdx <= finishPositionIdx) {
            for (; currentPosition != Processors.end(); ++currentPosition, ++currentPositionIdx) {
                (*currentPosition)->Execute(&message);
                if (currentPositionIdx == finishPositionIdx) {
                    break;
                }
            }
            finishPositionIdx = AtomicGet(ProcessorsCount) - 1;
            if (currentPositionIdx < finishPositionIdx) {
                ++currentPosition;
                ++currentPositionIdx;
            } else {
                break;
            }
        };
    }

    void TMessengerImpl::ClearProcessors() {
        DEBUG_LOG << "Messenger clearing..." << Endl;
        TWriteGuard wgSendMessage(SendMessageMutex);
        TWriteGuard wgUnregisterMessenger(UnregisterMessengerMutex);
        TGuard<TMutex> gModifyMessenger(MutexModifyMessenger);
        for (auto iter = Processors.begin(); iter != Processors.end();) {
            if ((*iter)->GetIsUnused()) {
                DEBUG_LOG << "Remove " << (*iter)->Name() << " processor" << Endl;
                iter = Processors.erase(iter);
            } else {
                ++iter;
            }
        }
        AtomicSet(ProcessorsCount, Processors.size());
        DEBUG_LOG << "Messenger clearing...OK" << Endl;
    }

    void TMessengerImpl::RegisterMessageProcessor(IMessageProcessor* messageProcessor) {
        TGuard<TMutex> gModifyMessenger(MutexModifyMessenger);
        DEBUG_LOG << "Register " << messageProcessor->Name() << ". Address: " << (const void*)messageProcessor << Endl;
        for (TProcessorsList::iterator i = Processors.begin(), e = Processors.end(); i != e; ++i) {
            if (!(*i)->GetIsUnused() && (*i)->IsContain(messageProcessor)) {
                FAIL_LOG("Secondary usage RegisterMessageProcessor for object: %s", messageProcessor->Name().data());
            }
        }
        Processors.push_back(MakeAtomicShared<TMessageProcessorExecutor>(messageProcessor));
        AtomicIncrement(ProcessorsCount);
        DEBUG_LOG << "Register " << messageProcessor->Name() << "...OK" << Endl;
    }

    void TMessengerImpl::UnRegisterMessageProcessor(IMessageProcessor* messageProcessor) {
        DEBUG_LOG << "UnRegister " << messageProcessor->Name() << ". Address: " << (const void*)messageProcessor << Endl;
        TReadGuard rgUnregisterMessenger(UnregisterMessengerMutex);
        for (TProcessorsList::iterator i = Processors.begin(), e = Processors.end(); i != e; ++i) {
            if (!(*i)->GetIsUnused() && (*i)->IsContain(messageProcessor)) {
                DEBUG_LOG << "UnRegister " << messageProcessor->Name() << "... OK" << Endl;
                (*i)->MarkAsUnused();
                return;
            }
        }
        Print();
        FAIL_LOG("Incorrect usage UnRegisterMessageProcessor");
    }

}
