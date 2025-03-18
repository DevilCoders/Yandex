#include "phone.h"

////
//  TPhone
////

TPhone::TPhone(const TString& country,
               const TString& area,
               const TString& local)
    : CountryCode(country)
    , AreaCode(area)
    , LocalPhone(local)
{
}

TString TPhone::ToPhoneWithArea() const {
    if (!AreaCode.empty()) {
        return TString().append('(').append(AreaCode).append(')').append(LocalPhone);
    } else {
        return LocalPhone;
    }
}

TString TPhone::ToPhoneWithCountry() const {
    return CountryCode + ToPhoneWithArea();
}
