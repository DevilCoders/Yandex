#pragma once

#include "config.h"

#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <util/system/mutex.h>

namespace NCS {

    class TPQFake: public IPQClient {
    private:
        using TBase = IPQClient;
        const TPQFakeConfig Config;
        mutable TMap<TString, IPQMessage::TPtr> Messages;
        mutable TMutex Mutex;
    protected:
        virtual bool DoStartImpl() override {
            return true;
        }
        virtual bool DoStopImpl() override {
            return true;
        }

        virtual bool DoReadMessages(TVector<IPQMessage::TPtr>& result, ui32* /*failedMessages*/, const size_t maxSize, const TDuration /*timeout*/) const override;
        virtual IPQClient::TPQResult DoWriteMessage(const IPQMessage& message) const override;
        virtual IPQClient::TPQResult DoAckMessage(const IPQMessage& msg) const override;
        virtual bool DoFlushWritten() const override;
        virtual bool IsReadable() const override;
        virtual bool IsWritable() const override;
    public:
        TPQFake(const TString& clientId, const TPQFakeConfig& config, const IPQConstructionContext& context);
    };
}
