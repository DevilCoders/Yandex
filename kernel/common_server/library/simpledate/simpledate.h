#pragma once
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/util/instant_model.h>
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {
    class TSimpleDate {
    private:
        CS_ACCESS(TSimpleDate, ui16, Year, 1990);
        CS_ACCESS(TSimpleDate, ui8, Month, 1);
        CS_ACCESS(TSimpleDate, ui8, Day, 10);

    public:
        TSimpleDate() = default;
        TSimpleDate(const TInstant instant);

        TString ToIso8601() const;
        TString ToDMY() const;
        TString GetStringHashDataInternal() const;
        TString ToString() const;

        static NFrontend::TScheme GetScheme(const IBaseServer& /*server*/);

        static TSimpleDate Now() {
            return TSimpleDate(ModelingNow());
        }

        bool SimpleValidation() const;

        template <class TProto>
        Y_WARN_UNUSED_RESULT bool DeserializeFromProto(const TProto& proto) {
            if (sscanf(proto.GetDate().c_str(), "%" SCNu16 "-%" SCNu8 "-%" SCNu8, &Year, &Month, &Day) != 3) {
                Year = proto.GetYear();
                Month = proto.GetMonth();
                Day = proto.GetDay();
            }
            return SimpleValidation();
        }

        template <class TProto>
        void SerializeToProto(TProto& proto) const {
            proto.SetYear(Year);
            proto.SetMonth(Month);
            proto.SetDay(Day);
        }
    };

    bool operator==(const TSimpleDate& l, const TSimpleDate& r);
    bool operator<(const TSimpleDate& l, const TSimpleDate& r);
    bool operator>(const TSimpleDate& l, const TSimpleDate& r);

} // namespace NCS
