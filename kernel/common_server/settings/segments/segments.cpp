#include "segments.h"

namespace NCS {

    bool TSegmentedSettings::DoStart() {
        for (auto&& i : SettingsOperators) {
            if (i.GetSegmentId() == "default") {
                return true;
            }
        }
        return false;
    }

    bool TSegmentedSettings::DoStop() {
        return true;
    }

    TSegmentedSettings::~TSegmentedSettings() {
        Stop();
    }

    void TSegmentedSettings::RegisterSegment(const TString& segmentId, ISettings::TPtr segment) {
        CHECK_WITH_LOG(!!segment) << "incorrect settings segment: " << segmentId << Endl;
        for (auto&& i : SettingsOperators) {
            CHECK_WITH_LOG(i.GetSegmentId() != segmentId) << "settings segment id duplication: " << segmentId << Endl;
        }
        SettingsOperators.emplace_back(segmentId, segment);
        std::sort(SettingsOperators.begin(), SettingsOperators.end());
    }

    bool TSegmentedSettings::GetValueStr(const TString& key, TString& result) const {
        if (!IsActive()) {
            return false;
        }
        for (auto&& i : SettingsOperators) {
            if (i.IsMine(key)) {
                return i->GetValueStr(key, result);
            }
        }
        S_FAIL_LOG << "Incorrect logic" << Endl;
        return false;
    }

    bool TSegmentedSettings::HasValues(const TSet<TString>& keys, TSet<TString>& existKeys) const {
        TMap<TString, TSet<TString>> keysBySegments = SplitBySegments<TSet<TString>, TString, TStringOperator>(keys);
        for (auto&& i : keysBySegments) {
            TSet<TString> lExistsKeys;
            if (!GetSegment(i.first)->HasValues(i.second, lExistsKeys)) {
                return false;
            }
            existKeys.insert(lExistsKeys.begin(), lExistsKeys.end());
        }
        return true;
    }

    bool TSegmentedSettings::RemoveKeys(const TVector<TString>& keys, const TString& userId) const {
        TMap<TString, TSet<TString>> keysBySegments = SplitBySegments<TVector<TString>, TString, TStringOperator>(keys);
        for (auto&& i : keysBySegments) {
            TVector<TString> vKeys(i.second.begin(), i.second.end());
            if (!GetSegment(i.first)->RemoveKeys(vKeys, userId)) {
                return false;
            }
        }
        return true;
    }

    bool TSegmentedSettings::SetValues(const TVector<TSetting>& values, const TString& userId) const {
        TMap<TString, TSet<TSetting>> settingBySegments = SplitBySegments<TVector<TSetting>, TSetting, TSettingOperator>(values);
        for (auto&& i : settingBySegments) {
            TVector<TSetting> vObjs(i.second.begin(), i.second.end());
            if (!GetSegment(i.first)->SetValues(vObjs, userId)) {
                return false;
            }
        }
        return true;
    }

    bool TSegmentedSettings::GetHistory(const TInstant since, TVector<TAtomicSharedPtr<TObjectEvent<TSetting>>>& result) const {
        for (auto&& i : SettingsOperators) {
            TVector<TAtomicSharedPtr<TObjectEvent<TSetting>>> lEvents;
            if (!i->GetHistory(since, lEvents)) {
                return false;
            }
            result.insert(result.end(), lEvents.begin(), lEvents.end());
        }
        const auto predSort = [](TAtomicSharedPtr<TObjectEvent<TSetting>> l, TAtomicSharedPtr<TObjectEvent<TSetting>> r) {
            return l->GetHistoryInstant() < r->GetHistoryInstant();
        };
        std::sort(result.begin(), result.end(), predSort);
        return true;
    }

    bool TSegmentedSettings::GetAllSettings(TVector<TSetting>& result) const {
        for (auto&& i : SettingsOperators) {
            TVector<TSetting> lSettings;
            if (!i->GetAllSettings(lSettings)) {
                return false;
            }
            result.insert(result.end(), lSettings.begin(), lSettings.end());
        }
        return true;
    }

    bool TSettingsSegment::operator<(const TSettingsSegment& segment) const {
        if (SegmentId == "default" && segment.SegmentId != "default") {
            return false;
        } else if (SegmentId != "default" && segment.SegmentId == "default") {
            return true;
        } else if (SegmentId.size() < segment.SegmentId.size()) {
            return false;
        } else if (SegmentId.size() == segment.SegmentId.size()) {
            return SegmentId < segment.SegmentId;
        } else {
            return true;
        }
    }

    bool TSettingsSegment::IsMine(const TString& key) const {
        if (SegmentId == "default") {
            return true;
        }
        if (!key.StartsWith(SegmentId)) {
            return false;
        }
        if (key.size() == SegmentId.size()) {
            return true;
        }
        return key[SegmentId.size()] == '.';
    }

}
