#include "queue.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {
    TLogbrokerClient::TLogbrokerClient(const TString& clientId, const TLogbrokerClientConfig& config, const IPQConstructionContext& context)
        : TBase(clientId)
        , Config(config)
        , ConstructionContext(context)
    {}

    bool TLogbrokerClient::DoStartImpl() {
        auto driverConfig = Config.BuildDriverConfig();
        if (!Config.GetAuthConfig().PatchDriverConfig(driverConfig, ConstructionContext.GetTvmManager())) {
            return false;
        }
        Driver = MakeHolder<NYdb::TDriver>(driverConfig);

        PQClient = MakeHolder <NYdb::NPersQueue::TPersQueueClient>(*Driver, Config.BuildPQClientConfig());

        if (Config.HasWriteConfig()) {
            Writer = MakeHolder<TLogbrokerWriter>(Config.GetWriteConfigUnsafe());
            if (!Writer->Start(*PQClient)) {
                return false;
            }
        }

        if (Config.HasReadConfig()) {
            Reader = MakeHolder<TLogbrokerReader>(Config.GetReadConfigUnsafe());
            if (!Reader->Start(*PQClient, Config.GetInteractionTimeout())) {
                return false;
            }
        }

        if (!Reader && !Writer) {
            TFLEventLog::Error("Read or Write section must be set");
            return false;
        }
        return true;
    }

    bool TLogbrokerClient::DoStopImpl() {
        if (Writer && !Writer->Stop()) {
            return false;
        }
        Writer.Destroy();

        if (Reader && !Reader->Stop()) {
            return false;
        }
        Reader.Destroy();

        PQClient.Destroy();

        Driver->Stop(true);
        Driver.Destroy();
        return true;
    }

    bool TLogbrokerClient::DoReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages, const size_t maxSize, const TDuration timeout) const {
        return Reader->ReadMessages(result, failedMessages, maxSize, timeout);
    }

    IPQClient::TPQResult TLogbrokerClient::DoAckMessage(const IPQMessage& message) const {
        return Reader->AckMessage(message, Config.GetInteractionTimeout());
    }

    IPQClient::TPQResult TLogbrokerClient::DoWriteMessage(const IPQMessage& msg) const {
        return Writer->WriteMessage(msg, Config.GetInteractionTimeout());
    }

    bool TLogbrokerClient::DoFlushWritten() const {
        return Writer->FlushWritten(Config.GetInteractionTimeout());
    }

    bool TLogbrokerClient::IsReadable() const {
        return !!Reader;
    }

    bool TLogbrokerClient::IsWritable() const {
        return !!Writer;
    }

}
