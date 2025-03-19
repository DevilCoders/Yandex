#pragma once

#include <util/generic/fwd.h>
#include <util/generic/vector.h>
#include <kernel/common_server/util/accessor.h>

#include <contrib/libs/libphonenumber/cpp/src/phonenumbers/phonenumberutil.h>

class TPhoneNormalizer {
public:
    using TLocalCountryCodes = TVector<TString>;

    static const TString DefaultRegionCode;
    static const TVector<TString> DefaultLocalCountryCodes;

private:
    RTLINE_READONLY_ACCEPTOR(AdmissibleLocalCountryCodes, TLocalCountryCodes, DefaultLocalCountryCodes);

public:
    explicit TPhoneNormalizer(const TLocalCountryCodes& codes = DefaultLocalCountryCodes);

    TString TryNormalize(const TString& rawNumber,
                         const TString& country = DefaultRegionCode,
                         i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat format = i18n::phonenumbers::PhoneNumberUtil::E164);

    TVector<TString> Normalize(const TString& rawNumber,
                               const TString& country = DefaultRegionCode,
                               i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat format = i18n::phonenumbers::PhoneNumberUtil::E164);

private:
    bool Parse(const TString& number,
               const TString& country,
               i18n::phonenumbers::PhoneNumber& phoneNumber,
               i18n::phonenumbers::PhoneNumberUtil* u);

    void TryTrimPrefixes(TString rawNumber, TVector<TString>& possibleRawNumbers);
};
