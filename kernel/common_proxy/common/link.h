#pragma once

#include "data_set.h"
#include "replier.h"
#include "counter.h"
#include "link_config.h"

#include <kernel/common_proxy/unistat_signals/signals.h>
#include <library/cpp/mediator/messenger.h>
#include <library/cpp/bucket_quoter/bucket_quoter.h>

namespace NCommonProxy {

    class TProcessor;

    class TLink : public TAtomicRefCount<TLink>, public IMessageProcessor, public IUnistatSignalSource {
    public:
        using TPtr = TIntrusivePtr<TLink>;
        class IFilter {
        public:
            virtual ~IFilter() {}
            virtual bool Accepted(const TDataSet::TPtr input) const = 0;
        };

    public:
        TLink(const TProcessor& from, const TProcessor& to, const TLinkConfig& config);
        ~TLink();
        void ForwardRequest(TDataSet::TPtr input, IReplier::TPtr replier) const;
        virtual bool Process(IMessage* message) override;
        virtual TString Name() const override;
        virtual void RegisterSignals(TUnistat& tass) const override;
        virtual void CollectInfo(NJson::TJsonValue& result) const;

        template <typename T, typename... Args>
        void SetFilter(Args&&... args) {
            Filter = MakeHolder<T>(std::forward<Args>(args)...);
        }

    private:
        const TLinkConfig& Config;
        const TProcessor& From;
        const TProcessor& To;
        mutable TCounter ForwardedCounter;
        mutable TCounter DroppedCounter;
        TString LinkName;
        THolder<IFilter> Filter;
        mutable TBucketQuoter<int> Quoter;
    };

}
