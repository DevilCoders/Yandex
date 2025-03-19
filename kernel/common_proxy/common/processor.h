#pragma once

#include "data_set.h"
#include "processor_config.h"
#include "counter.h"
#include "link.h"
#include "replier.h"

#include <kernel/common_proxy/unistat_signals/signals.h>
#include <library/cpp/mediator/messenger.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/yconf/conf.h>
#include <util/thread/pool.h>
#include <util/system/event.h>

namespace NCommonProxy {

    class IStrictDataProcessor {
    public:
        virtual ~IStrictDataProcessor() {}

        virtual const TMetaData& GetInputMetaData() const;
        virtual const TMetaData& GetOutputMetaData() const;

    protected:
        TMetaData InputMeta;
        TMetaData OutputMeta;
    };

    class TProcessor : public TAtomicRefCount<TProcessor>, public IMessageProcessor, public IStrictDataProcessor, public IUnistatSignalSource {
    public:
        typedef TIntrusivePtr<TProcessor> TPtr;
        typedef NObjectFactory::TParametrizedObjectFactory<TProcessor, TString, const TString&, const TProcessorsConfigs&> TFactory;

    public:
        virtual ~TProcessor();
        static TPtr Create(const TString& name, const TProcessorsConfigs& config);

        TString GetName() const;
        void AddListener(TLink::TPtr link);
        virtual void AddRequester(TLink::TPtr link);
        void Process(TDataSet::TPtr input, IReplier::TPtr replier) const;
        virtual void Init();
        void Start();
        virtual void Run();
        void Stop();
        void Wait();
        //IMessageProcessor
        virtual TString Name() const override final;
        virtual bool Process(IMessage* message) override;
        virtual void RegisterSignals(TUnistat& tass) const override final;
        // ThreadPoolBinder
        void* CreateThreadSpecificResource() const;
        void DestroyThreadSpecificResource(void*) const;

    protected:
        TProcessor(const TString& name, const TProcessorsConfigs& configs);
        virtual void DoProcess(TDataSet::TPtr input, IReplier::TPtr replier) const = 0;
        virtual bool DoProcessMessage(IMessage* /*message*/) { return false; }
        virtual void DoStart() = 0;
        virtual void DoStop() = 0;
        virtual void DoWait() = 0;
        virtual void CollectInfo(NJson::TJsonValue& result) const;
        virtual void UpdateUnistatSignals() const;
        void SendRequestToListeners(TDataSet::TPtr input, IReplier::TPtr replier) const;
        virtual void DoRegisterSignals(TUnistat& /*tass*/) const {}

    protected:
        mutable TCounter ProcessedCounter;

    private:
        void ProcessImpl(TDataSet::TPtr input, IReplier::TPtr replier, TInstant enqueued) const;
        void PushProcessorSignal(const TString& processor, const TString& signalName, double value) const;

    private:
        const TProcessorConfig& Config;
        TVector<TLink::TPtr> Links;
        mutable TThreadPoolBinder<TThreadPool, TProcessor> Queue;
        mutable TManualEvent Started;
        mutable TAtomic SpendTime = 0;
        TString ThreadsName;
    };

}
