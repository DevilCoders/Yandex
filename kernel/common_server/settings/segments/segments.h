#pragma once
#include <library/cpp/mediator/messenger.h>
#include <kernel/common_server/util/accessor.h>
#include <util/datetime/base.h>
#include <kernel/common_server/api/history/event.h>
#include <kernel/common_server/common/abstract.h>
#include <library/cpp/json/writer/json_value.h>
#include <kernel/common_server/settings/abstract/abstract.h>
#include <kernel/common_server/util/auto_actualization.h>

namespace NCS {
    class TSettingsSegment {
    private:
        CSA_DEFAULT(TSettingsSegment, TString, SegmentId);
        CSA_DEFAULT(TSettingsSegment, ISettings::TPtr, Settings);
    public:
        TSettingsSegment(const TString& id, ISettings::TPtr segment)
            : SegmentId(id)
            , Settings(segment)
        {

        }
        const ISettings* operator->() const {
            return Settings.Get();
        }
        bool operator<(const TSettingsSegment& segment) const;
        bool IsMine(const TString& key) const;
    };

    class TSegmentedSettings: public ISettings, public IStartStopProcess {
    private:
        using TBase = ISettings;
        TVector<TSettingsSegment> SettingsOperators;
        ISettings::TPtr GetSegment(const TString& key) const {
            for (auto&& i : SettingsOperators) {
                if (i.IsMine(key)) {
                    return i.GetSettings();
                }
            }
            CHECK_WITH_LOG(false) << "Incorrect logic" << Endl;
            return nullptr;
        }

        class TStringOperator {
        public:
            static const TString& GetKey(const TString& key) {
                return key;
            }
        };

        class TSettingOperator {
        public:
            static const TString& GetKey(const TSetting& s) {
                return s.GetKey();
            }
        };

        template <class TContainer, class TObject, class TObjectOperator>
        TMap<TString, TSet<TObject>> SplitBySegments(const TContainer& objects) const {
            TMap<TString, TSet<TObject>> result;
            TSet<TObject> defaultObjects(objects.begin(), objects.end());
            for (auto&& i : SettingsOperators) {
                TSet<TObject> lObjects;
                for (auto&& obj : objects) {
                    if (i.IsMine(TObjectOperator::GetKey(obj))) {
                        lObjects.emplace(obj);
                        defaultObjects.erase(obj);
                    }
                }
                if (lObjects.size()) {
                    CHECK_WITH_LOG(result.emplace(i.GetSegmentId(), std::move(lObjects)).second);
                }
            }
            CHECK_WITH_LOG(defaultObjects.empty());
            return result;
        }
    protected:
        virtual bool DoStart() override;
        virtual bool DoStop() override;
    public:
        using TBase::TBase;
        ~TSegmentedSettings();

        void RegisterSegment(const TString& segmentId, ISettings::TPtr segment);
        virtual bool GetValueStr(const TString& key, TString& result) const override;
        virtual bool HasValues(const TSet<TString>& keys, TSet<TString>& existKeys) const override;

        virtual bool RemoveKeys(const TVector<TString>& keys, const TString& userId) const override;

        virtual bool SetValues(const TVector<TSetting>& values, const TString& userId) const override;
        virtual bool GetHistory(const TInstant since, TVector<TAtomicSharedPtr<TObjectEvent<TSetting>>>& result) const override;

        virtual bool GetAllSettings(TVector<TSetting>& result) const override;
    };
}
