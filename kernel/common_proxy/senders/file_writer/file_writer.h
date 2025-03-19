#pragma once

#include <kernel/common_proxy/common/sender.h>
#include <util/system/mutex.h>

namespace NCommonProxy {

    class TFileWriter : public TSender {
    public:
        class TConfig : public TProcessorConfig {
        public:
            TConfig(const TString& type)
                : TProcessorConfig(type)
            {}

            static TProcessorConfig::TFactory::TRegistrator<TConfig> Registrator;
            TFsPath OutputFile;

        protected:
            virtual bool DoCheck() const override;
            virtual void DoInit(const TYandexConfig::Section& componentSection) override;
            virtual void DoToString(IOutputStream& so) const  override;
        };

    public:
        TFileWriter(const TString& name, const TProcessorsConfigs& configs);
        virtual void DoStart() override;
        virtual void DoStop() override;
        virtual void DoWait() override;

        virtual const TMetaData& GetInputMetaData() const override;
        virtual const TMetaData& GetOutputMetaData() const override;

        static TProcessor::TFactory::TRegistrator<TFileWriter> Registarar;

    protected:
        virtual void DoProcess(TDataSet::TPtr input, IReplier::TPtr replier) const override;

    private:
        class TInputMetaData;
        const TConfig& Config;
        THolder<IOutputStream> Output;
        TMutex Mutex;
    };

}
