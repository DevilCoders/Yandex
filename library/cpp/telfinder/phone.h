#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

////
//  TPhone
////

class TPhone {
private:
    TString CountryCode;
    TString AreaCode;
    TString LocalPhone;

public:
    TPhone() {
    }
    TPhone(const TString& country, const TString& area, const TString& local);

    inline void SetAreaCode(const TString& areaCode) {
        AreaCode = areaCode;
    }
    inline void SetCountryCode(const TString& countryCode) {
        CountryCode = countryCode;
    }

    inline const TString& GetAreaCode() const {
        return AreaCode;
    }
    inline const TString& GetCountryCode() const {
        return CountryCode;
    }
    inline const TString& GetLocalPhone() const {
        return LocalPhone;
    }
    TString ToPhoneWithArea() const;
    TString ToPhoneWithCountry() const;
};

////
//  TPhoneLocation
////

struct TPhoneLocation {
    std::pair<size_t, size_t> PhonePos;

    std::pair<size_t, size_t> CountryCodePos;
    std::pair<size_t, size_t> AreaCodePos;
    std::pair<size_t, size_t> LocalPhonePos;

    TPhoneLocation()
        : PhonePos(0, 0)
        , CountryCodePos(0, 0)
        , AreaCodePos(0, 0)
        , LocalPhonePos(0, 0)
    {
    }
};

////
//  TFoundPhone
////

struct TFoundPhone {
    TPhone Phone;
    TPhoneLocation Location;
    TString Structure;
};

using TFoundPhones = TVector<TFoundPhone>;
