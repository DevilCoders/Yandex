#include "link.h"
#include "processor.h"
#include <kernel/daemon/messages.h>

namespace NCommonProxy {

    TLink::TLink(const TProcessor& from, const TProcessor& to, const TLinkConfig& config)
        : Config(config)
        , From(from)
        , To(to)
        , ForwardedCounter("forwarded")
        , DroppedCounter("dropped")
        , LinkName(From.GetName() + "->" + To.GetName())
        , Quoter(Config.RpsLimit, 10 * Config.RpsLimit, nullptr, nullptr, nullptr)
    {
        const TMetaData& outputMD = From.GetOutputMetaData();
        const TMetaData& inputMD = To.GetInputMetaData();
        if (!inputMD.IsSubsetOf(outputMD))
            ythrow yexception() << To.GetName() << " cannot listen " << From.GetName()
                << " because it not generate all needed data: there is " << outputMD.ToString()
                << ", must be " << inputMD.ToString();
        RegisterGlobalMessageProcessor(this);
    }

    TLink::~TLink() {
        UnregisterGlobalMessageProcessor(this);
    }

    void TLink::ForwardRequest(TDataSet::TPtr input, IReplier::TPtr replier) const {
        if (!Filter || Filter->Accepted(input)) {
            if (Config.RpsLimit) {
                Quoter.Use(1, true);
            }
            To.Process(input, replier);
            if (!Config.IsIgnoredSignal("send_to_")) {
                TCommonProxySignals::PushSpecialSignal(From.GetName(), "send_to_" + To.GetName(), 1);
            }
            ForwardedCounter.Hit();
        } else {
            if (!Config.IsIgnoredSignal("not_send_to_")) {
                TCommonProxySignals::PushSpecialSignal(From.GetName(), "not_send_to_" + To.GetName(), 1);
            }
            DroppedCounter.Hit();
        }
    }

    bool TLink::Process(IMessage* message) {
        if (TCollectServerInfo* info = dynamic_cast<TCollectServerInfo*>(message)) {
            CollectInfo(info->Fields["links"][Name()]);
            return true;
        }
        return false;
    }

    TString TLink::Name() const {
        return LinkName;
    }

    void TLink::RegisterSignals(TUnistat& tass) const {
        if (!Config.IsIgnoredSignal("send_to_")) {
            tass.DrillFloatHole(TCommonProxySignals::GetSignalName(From.GetName(), "send_to_" + To.GetName()),
                "dmmm", NUnistat::TPriority(50));
        }
        if (!Config.IsIgnoredSignal("not_send_to_")) {
            tass.DrillFloatHole(TCommonProxySignals::GetSignalName(From.GetName(), "not_send_to_" + To.GetName()),
                "dmmm", NUnistat::TPriority(50));
        }
    }

    void TLink::CollectInfo(NJson::TJsonValue& result) const {
        ForwardedCounter.WriteInfo(result);
        DroppedCounter.WriteInfo(result);
    }

}
