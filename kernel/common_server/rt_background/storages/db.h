#pragma once
#include <kernel/common_server/api/history/cache.h>
#include "abstract.h"

namespace NBServer {
    class TDBRTBackgroundProcessesStorageConfig: public IRTBackgroundStorageConfig {
    private:
        static TFactory::TRegistrator<TDBRTBackgroundProcessesStorageConfig> Registrator;
        TString DBName;
        THistoryConfig HistoryConfig;
    public:
        virtual TString GetClassName() const override {
            return GetTypeName();
        }
        static TString GetTypeName() {
            return "db";
        }
        virtual void Init(const TYandexConfig::Section* section) override;
        virtual void ToString(IOutputStream& os) const override;
        virtual THolder<IRTBackgroundProcessesStorage> BuildStorage(const IBaseServer& server) const override;
    };

    class TBackgroundHistoryManager: public TIndexedAbstractHistoryManager<TRTBackgroundProcessContainer> {
    private:
        using TBase = TIndexedAbstractHistoryManager<TRTBackgroundProcessContainer>;
    protected:
        using TBase::TBase;

    public:
        TBackgroundHistoryManager(const IHistoryContext& context, const THistoryConfig& config)
            : TBase(context, "rt_background_settings_history", config) {
        }
    };

    class TDBRTBackgroundProcessesStorage:
        public IRTBackgroundProcessesStorage,
        public TDBCacheWithHistoryOwner<TBackgroundHistoryManager, TRTBackgroundProcessContainer>,
        public TNonCopyable {
    private:
        using TBase = TDBCacheWithHistoryOwner<TBackgroundHistoryManager, TRTBackgroundProcessContainer>;
    protected:
        virtual TStringBuf GetEventObjectId(const TObjectEvent<TRTBackgroundProcessContainer>& ev) const override;
        virtual bool DoRebuildCacheUnsafe() const override;
        virtual void AcceptHistoryEventUnsafe(const TObjectEvent<TRTBackgroundProcessContainer>& ev) const override;
    public:
        using TBase::TBase;
        virtual bool GetObjects(TMap<TString, TRTBackgroundProcessContainer>& settings) const override;
        virtual bool RemoveBackground(const TVector<TString>& processIds, const TString& userId) const override;
        virtual bool SetBackgroundEnabled(const TSet<TString>& bgNames, const bool enabled, const TString& userId) const override;
        virtual bool UpsertBackgroundSettings(const TRTBackgroundProcessContainer& process, const TString& userId) const override;
        virtual bool ForceUpsertBackgroundSettings(const TRTBackgroundProcessContainer& process, const TString& userId) const override;
        virtual bool StartStorage() override {
            return TBase::Start();
        }
        virtual bool StopStorage() override {
            return TBase::Stop();
        }
    };

}
