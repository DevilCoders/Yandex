#pragma once

#include <util/stream/str.h>
#include <library/cpp/string_utils/scan/scan.h>
#include <util/string/strip.h>
#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/hash.h>
#include <util/string/split.h>
#include <library/cpp/logger/global/global.h>

namespace NUtil {

class TKeyValueLogRecord {
public:
    TKeyValueLogRecord(const TString& delimiter, const TString& kvDelimeter)
        : Delimiter(delimiter)
        , KeyValueDelimiter(kvDelimeter)
    {
    }

    template <class T>
    inline void Add(const TString& key, const T& value) {
        if (!!value) {
            ForceAdd(key, value);
        }
    }
    template <class T>
    inline void ForceAdd(const TString& key, const T& value) {
        Stream << Delimiter;
        AddKeyValue(key, value);
    }
    inline void AddStandaloneKey(const TString& key) {
        Stream << key;
    }
    inline TString ToString() const {
        return Stream.Str();
    }
    template <class T>
    void operator()(const TString& key, const T& value) {
        Add(key, value);
    }

private:
    template <class T>
    inline void AddKeyValue(const TString& key, const T& value) {
        Stream << key << KeyValueDelimiter << value;
    }

private:
    TStringStream Stream;
    TString Delimiter;
    TString KeyValueDelimiter;
};

class TTSKVRecord: public TKeyValueLogRecord {
public:
    TTSKVRecord(const TString& format = Default<TString>())
        : TKeyValueLogRecord("\t", "=")
    {
        AddStandaloneKey("tskv");
        if (format) {
            Add("tskv_format", format);
        }
    }
};

class TTSKVRecordParser {
public:
    template <char sep = '\t', char sepKV = '='>
    static void Parse(TStringBuf record, TMap<TString, TString>& result) {
        result.clear();
        const auto saver = [&result](const TStringBuf key, const TStringBuf val) {
            result[(TString)key] = val;
        };
        ScanKeyValue<true, sep, sepKV>(record, saver);
    }

    template <char sep = '\t', char sepKV = '='>
    static TMap<TString, TString> Parse(TStringBuf record) {
        TMap<TString, TString> result;
        Parse<sep, sepKV>(record, result);
        return result;
    }
};



template <class TKeyType = TString>
class TCSVRemapper {
private:
    TMap<TKeyType, ui32> NameToIdx;
    TVector<TStringBuf> Context;
    ui32 MaxIdx = 0;
    const char Delim;
    const THashSet<TKeyType> RequiredFields;

public:
    TCSVRemapper(const TMap<TKeyType, ui32>& nameToIdx, const char delim, const THashSet<TKeyType>& requiredFields = {})
        : NameToIdx(nameToIdx)
        , Delim(delim)
        , RequiredFields(requiredFields)
    {
        for (auto&& [_, idx] : NameToIdx) {
            MaxIdx = Max<ui32>(MaxIdx, idx);
        }
    }

    TCSVRemapper(const TVector<TKeyType>& fieldNames, const char delim, const THashSet<TKeyType>& requiredFields = {})
        : Delim(delim)
        , RequiredFields(requiredFields)
    {
        for (ui32 i = 0; i < fieldNames.size(); ++i) {
            NameToIdx[fieldNames[i]] = i;
        }
        MaxIdx = NameToIdx.size() - 1;
    }

    const TVector<TStringBuf>& GetContext() const {
        return Context;
    }

    bool HasField(const TKeyType& fieldName) const {
        return NameToIdx.contains(fieldName);
    }

    bool IsRequired(const TKeyType& fieldName) const {
        return RequiredFields.contains(fieldName);
    }

    TMaybe<ui32> GetIndex(const TKeyType& fieldName) const {
        if (const auto* x = NameToIdx.FindPtr(fieldName)) {
            return *x;
        } else {
            return {};
        }
    }

    bool BuildContext(const TString& str, TVector<TStringBuf>& context) const {
        if (str.empty()) {
            return false;
        }
        auto start = str.cbegin();
        bool brakets = false;

        for (auto p = start; p != str.cend(); ++p) {
            if (*p == '"' && !brakets) {
                brakets = true;
            }
            if (*p == Delim) {
                if (brakets) {
                    if (*(p - 1) == '"') {
                        context.emplace_back(start + 1, p - 1); // Ignore '"'
                        brakets = false;
                        start = p + 1;
                    }
                } else {
                    context.emplace_back(start, p);
                    start = p + 1;
                }
            }
        }
        CHECK_WITH_LOG(start <= str.cend());
        if (brakets) {
            context.emplace_back(start + 1, str.cend() - 1);
        }  else {
            context.emplace_back(start, str.cend());
        }

        if (context.size() <= MaxIdx) {
            ERROR_LOG << "Problems on parsing: " << str << Endl;
            return false;
        }
        return true;
    }

    bool SetContext(const TString& str) {
        Context.clear();
        return BuildContext(str, Context);
    }

    template <class T>
    Y_FORCE_INLINE T GetValue(const TKeyType& fieldName) const {
        try {
            return FromString<T>(StripString(GetValueAsIs(fieldName)));
        } catch (...) {
            ythrow yexception() << "Cannot get " << fieldName << ": " << CurrentExceptionMessage();
        }
    }

    template <class T>
    Y_FORCE_INLINE bool TryGetValue(const TKeyType& fieldName, T& value) const {
        auto it = NameToIdx.find(fieldName);
        if (it == NameToIdx.end()) {
            return false;
        }
        CHECK_WITH_LOG(Context.size() > it->second) << Context.size() << "/" << it->second;
        return TryFromString<T>(StripString(Context[it->second]), value);
    }

    Y_FORCE_INLINE TStringBuf GetValueAsIs(const TKeyType& fieldName) const {
        return GetValueAsIs(fieldName, Context);
    }


    Y_FORCE_INLINE TStringBuf GetValueAsIs(const TKeyType& fieldName, const TVector<TStringBuf>& context) const {
        auto it = NameToIdx.find(fieldName);
        if (it == NameToIdx.end()) {
            return Default<TStringBuf>();
        }
        CHECK_WITH_LOG(context.size() > it->second) << context.size() << "/" << it->second;
        return context[it->second];
    }
};

}
