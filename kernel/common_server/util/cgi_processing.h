#pragma once

#include <kernel/common_server/library/logging/events.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/datetime/base.h>
#include <util/generic/typetraits.h>

class TCgiProcessor {
private:
    static bool GetStrValue(const TCgiParameters& cgi, const TString& fieldName, TString& result, const bool mustBe) noexcept {
        TString strValue = cgi.Get(fieldName);
        if (strValue.empty() && mustBe) {
            TFLEventLog::Error("cgi.no_field")("field", fieldName);
            return false;
        }
        result = std::move(strValue);
        return true;
    }

    template <class T>
    static bool ReadFromString(const TString& fieldName, const TString& strValue, T& result) noexcept {
        if (!TryFromString(strValue, result)) {
            TFLEventLog::Error("cgi.bad_field_type")("field", fieldName)("expected_type", TypeName<decltype(result)>());
            return false;
        }
        return true;
    }

    template <>
    bool ReadFromString(const TString& fieldName, const TString& strValue, TInstant& result) noexcept {
        ui64 seconds = 0;
        if (TryFromString(strValue, seconds)) {
            result = TInstant::Seconds(seconds);
            return true;
        }
        if (!TInstant::TryParseIso8601(strValue, result)) {
            TFLEventLog::Error("cgi.bad_field_type")("field", fieldName)("expected_type", TypeName<decltype(result)>());
            return false;
        }
        return true;
    }

public:
    template <class T, class TMaybePolicy>
    static bool Read(const TCgiParameters& cgi, const TString& fieldName, TMaybe<T, TMaybePolicy>& result, const bool mustBe = false) noexcept {
        TString strValue;
        if (!GetStrValue(cgi, fieldName, strValue, mustBe)) {
            return false;
        }
        if (strValue.empty()) {
            return true;
        }
        T value = {};
        if (!TCgiProcessor::ReadFromString(fieldName, strValue, value)) {
            return false;
        }
        result = value;
        return true;
    }

    template <class T>
    static bool Read(const TCgiParameters& cgi, const TString& fieldName, T& result, const bool mustBe = false) noexcept {
        TString strValue;
        if (!GetStrValue(cgi, fieldName, strValue, mustBe)) {
            return false;
        }
        if (strValue.empty()) {
            return true;
        }
        if (!TCgiProcessor::ReadFromString(fieldName, strValue, result)) {
            return false;
        }
        return true;
    }

public:
    template <class T>
    static bool Read(const TCgiParameters& cgi, const TString& fieldName, TVector<T>& result, const bool mustBe = false) noexcept {
        TString strValue;
        if (!GetStrValue(cgi, fieldName, strValue, mustBe)) {
            return false;
        }
        if (strValue.empty()) {
            return true;
        }

        TVector<T> local;
        for (auto&& strItem : StringSplitter(strValue).SplitBySet(", ").SkipEmpty().ToList<TString>()) {
            T value = {};
            if (!TCgiProcessor::ReadFromString(fieldName, strItem, value)) {
                return false;
            }
            local.emplace_back(std::move(value));
        }

        std::swap(local, result);
        return true;
    }

    template <class T>
    static bool Read(const TCgiParameters& cgi, const TString& fieldName, TSet<T>& result, const bool mustBe = false) noexcept {
        TString strValue;
        if (!GetStrValue(cgi, fieldName, strValue, mustBe)) {
            return false;
        }
        if (strValue.empty()) {
            return true;
        }

        TSet<T> local;
        for (auto&& strItem : StringSplitter(strValue).SplitBySet(", ").SkipEmpty().ToList<TString>()) {
            T value = {};
            if (!TCgiProcessor::ReadFromString(fieldName, strItem, value)) {
                return false;
            }
            local.emplace(std::move(value));
        }

        std::swap(local, result);
        return true;
    }

};
