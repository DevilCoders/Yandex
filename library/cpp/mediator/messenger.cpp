#include "messenger.h"
#include "messenger_impl.h"
#include <library/cpp/logger/global/global.h>

#include <util/system/mutex.h>
#include <util/generic/map.h>
#include <util/system/guard.h>
#include <util/generic/list.h>
#include <util/generic/singleton.h>
#include <util/system/yassert.h>
#include <util/string/cast.h>
#include <util/system/thread.h>

#include <typeinfo>
#include <util/thread/pool.h>
#include <util/system/rwlock.h>

bool SendGlobalMessage(IMessage& message) {
    NMessenger::TGlobalMediator::SendMessage(message);
    return message.GetMessageIsProcessed();
}

void SendGlobalMessageAsync(IMessage::TPtr message) {
    NMessenger::TGlobalMediator::SendMessageAsync(message);
}

void RegisterGlobalMessageProcessor(NMessenger::IMessageProcessor* processor) {
    Y_ASSERT(processor);
    NMessenger::TGlobalMediator::RegisterMessageProcessor(processor);
}

void UnregisterGlobalMessageProcessor(NMessenger::IMessageProcessor* processor) {
    Y_ASSERT(processor);
    NMessenger::TGlobalMediator::UnRegisterMessageProcessor(processor);
}

namespace NMessenger {
    IMessenger* IMessenger::BuildMessenger() {
        return new TMessengerImpl;
    }

    void TGlobalMediator::SendMessage(IMessage& message) {
        Singleton<TMessengerImpl>()->SendMessage(message);
    }

    void TGlobalMediator::SendMessageAsync(IMessage::TPtr message) {
        Singleton<TMessengerImpl>()->SendMessageAsync(message);
    }

    void TGlobalMediator::RegisterMessageProcessor(IMessageProcessor* messageProcessor) {
        Singleton<TMessengerImpl>()->RegisterMessageProcessor(messageProcessor);
    }

    void TGlobalMediator::UnRegisterMessageProcessor(IMessageProcessor* messageProcessor) {
        Singleton<TMessengerImpl>()->UnRegisterMessageProcessor(messageProcessor);
    }

    void TGlobalMediator::CheckEmpty() {
        Singleton<TMessengerImpl>()->CheckEmpty();
    }

    void TGlobalMediator::CheckList() {
        Singleton<TMessengerImpl>()->CheckList();
    }

    void TGlobalMediator::CheckList(const TString& name) {
        Singleton<TMessengerImpl>()->CheckList(name);
    }

}
