#include <contrib/libs/libphonenumber/cpp/src/phonenumbers/region_code.h>

#include <kernel/common_server/util/raw_text/phone_number.h>
#include <util/string/ascii.h>

const TString TPhoneNormalizer::DefaultRegionCode = i18n::phonenumbers::RegionCode::GetUnknown();
const TVector<TString> TPhoneNormalizer::DefaultLocalCountryCodes = { "+7" };

TPhoneNormalizer::TPhoneNormalizer(const TLocalCountryCodes& codes)
    : AdmissibleLocalCountryCodes(codes)
{
}

TString TPhoneNormalizer::TryNormalize(const TString& rawNumber, const TString& country, i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat format) {
    auto validPhoneNumbers = Normalize(rawNumber, country, format);

    if (!validPhoneNumbers) {
        return rawNumber;
    }

    if (validPhoneNumbers.size() > 1) {
        for (auto& validPhoneNumber : validPhoneNumbers) {
            for (const auto& code : AdmissibleLocalCountryCodes) {
                if (validPhoneNumber.StartsWith(code)) {
                    return validPhoneNumber;
                }
            }
        }
    }

    return validPhoneNumbers.front();
}

TVector<TString> TPhoneNormalizer::Normalize(const TString& rawNumber, const TString& country, i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat format) {
    auto spacelessNumber = rawNumber;
    spacelessNumber.erase(std::remove_if(spacelessNumber.begin(),spacelessNumber.vend(), [](auto c) { return ::IsAsciiSpace(c); }), spacelessNumber.vend());

    TVector<TString> possibleRawNumbers;
    if (!spacelessNumber.StartsWith("+")) {
        TryTrimPrefixes(spacelessNumber, possibleRawNumbers);
    } else {
        possibleRawNumbers.push_back(spacelessNumber);
    }

    auto u = i18n::phonenumbers::PhoneNumberUtil::GetInstance();

    TVector<TString> validPhoneNumbers;
    for (const auto& number : possibleRawNumbers) {
        i18n::phonenumbers::PhoneNumber phoneNumber;
        try {
            if (Parse(number, country, phoneNumber, u)) {
                std::string result;
                u->Format(phoneNumber, format, &result);
                validPhoneNumbers.emplace_back(TString(result));
            }
        } catch (...) {
        }
    }

    return validPhoneNumbers;
}

bool TPhoneNormalizer::Parse(const TString& rawNumber, const TString& country, i18n::phonenumbers::PhoneNumber& parsedNumber, i18n::phonenumbers::PhoneNumberUtil* u) {
    return (i18n::phonenumbers::PhoneNumberUtil::NO_PARSING_ERROR == u->Parse(rawNumber, country, &parsedNumber) &&
            u->IsValidNumber(parsedNumber));
}

void TPhoneNormalizer::TryTrimPrefixes(TString rawNumber, TVector<TString>& possibleRawNumbers) {
    const static TVector<TString> extraPrefixes = {"98", "8"};  // check for "9810" and "810" afterwards

    size_t trimPrefixSize = 0;

    do {
        if (trimPrefixSize) {
            rawNumber = rawNumber.substr(trimPrefixSize);
            trimPrefixSize = 0;
        }

        possibleRawNumbers.push_back("+" + rawNumber);
        for (const auto& code : AdmissibleLocalCountryCodes) {
            possibleRawNumbers.push_back(code + rawNumber);
        }

        for (const auto& prefix : extraPrefixes) {
            if (rawNumber.StartsWith(prefix)) {
                trimPrefixSize = prefix.size();
                if (rawNumber.StartsWith(prefix + "10")) {
                    trimPrefixSize += 2;
                }
                break;
            }
        }
    } while (trimPrefixSize);
}
