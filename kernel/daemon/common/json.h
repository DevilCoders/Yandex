#pragma once

#include <library/cpp/json/json_value.h>

#include <util/generic/set.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NUtil {
    class TJsonFilter {
    public:
        typedef TSet<TString> TFilters;

    public:
        inline const TFilters& GetFilters() const {
            return Filters;
        }

        inline TFilters& GetFilters() {
            return Filters;
        }

        void AddFilters(const TCgiParameters& cgi, const TString& prefix = TString());

        template <class TSource, class TDest = TSource>
        inline bool Apply(const TSource& source, TDest& dest) const {
            NJson::TJsonValue tempSource;
            const NJson::TJsonValue& convertedSource = ConvertSource(source, tempSource);
            bool result = true;
            if (Filters.empty())
                dest = ConvertDest<TDest>(convertedSource);
            else {
                NJson::TJsonValue jsonDest(NJson::JSON_MAP);
                for (TFilters::const_iterator f = Filters.begin(); f != Filters.end(); ++f) {
                    const NJson::TJsonValue* value = convertedSource.GetValueByPath(*f, '.');
                    if (value)
                        jsonDest.SetValueByPath(*f, *value);
                    else if (!IgnoreUnknownPath) {
                        jsonDest.SetValueByPath(*f, "Incorrect path");
                        result = false;
                    }
                }
                dest = ConvertDest<TDest>(jsonDest);
            }
            return result;
        }

        inline void SetIgnoreUnknownPath(bool value) {
            IgnoreUnknownPath = value;
        }
    private:
        static const NJson::TJsonValue& ConvertSource(TStringBuf source, NJson::TJsonValue& tempSource);
        static const NJson::TJsonValue& ConvertSource(const NJson::TJsonValue& source, NJson::TJsonValue& /*tempSource*/);

        template <class TDest>
        static TDest ConvertDest(const NJson::TJsonValue& dest);

    private:
        TFilters Filters;
        bool IgnoreUnknownPath = false;
    };

    template <>
    inline TString TJsonFilter::ConvertDest<TString>(const NJson::TJsonValue& dest) {
        return dest.GetStringRobust();
    }

    template <>
    inline NJson::TJsonValue TJsonFilter::ConvertDest<NJson::TJsonValue>(const NJson::TJsonValue& dest) {
        return dest;
    }
}
