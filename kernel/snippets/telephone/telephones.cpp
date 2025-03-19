#include "telephones.h"

#include <kernel/tarc/iface/tarcface.h>

#include <util/generic/algorithm.h>
#include <util/charset/wide.h>

namespace NSnippets
{
    const TUtf16String TUserTelephones::TEL_FULL = u"tel_full";
    const TUtf16String TUserTelephones::TEL_LOCAL= u"tel_local";
    const TUtf16String TUserTelephones::TEL_CODE_AREA = u"tel_code_area";

    bool TUserTelephones::IsPhoneAttribute(const TUtf16String& attributeName)
    {
        return attributeName == TEL_FULL || attributeName == TEL_LOCAL || attributeName == TEL_CODE_AREA;
    }

    TPhone TUserTelephones::StringToPhone(const TString& phoneRepresentation)
    {
        const TString::size_type telCodeAreaBeg = phoneRepresentation.find('(');
        const TString::size_type telCodeAreaEnd = phoneRepresentation.find(')');
        const TString::size_type length = phoneRepresentation.length();

        TString codeCountry;
        TString codeArea;
        TString codeLocal;
        if (telCodeAreaBeg != TString::npos &&
            telCodeAreaEnd > telCodeAreaBeg &&
            telCodeAreaEnd + 1 < length)
        {
            codeCountry = phoneRepresentation.substr(0, telCodeAreaBeg);
            codeArea = phoneRepresentation.substr(telCodeAreaBeg + 1, telCodeAreaEnd - telCodeAreaBeg - 1);
            codeLocal = phoneRepresentation.substr(telCodeAreaEnd + 1);
        } else {
            codeLocal = phoneRepresentation;
        }

        return TPhone(codeCountry, codeArea, codeLocal);
    }

    TPhone TUserTelephones::StringToPhone(const TUtf16String& phoneRepresentation)
    {
        return StringToPhone(WideToASCII(phoneRepresentation));
    }

    TPhone TUserTelephones::AttrsToPhone(const THashMap<TString, TUtf16String>& telephoneAttributes)
    {
        typedef THashMap<TString, TUtf16String>::const_iterator TAttrConstIterator;

        TString local;
        const TAttrConstIterator telLocalAttr =
            telephoneAttributes.find(NArchiveZoneAttr::NTelephone::LOCAL_NUMBER);
        if (telLocalAttr != telephoneAttributes.end()) {
            local = WideToASCII(telLocalAttr->second);
        }

        TString area;
        const TAttrConstIterator telCodeAreaAttr =
            telephoneAttributes.find(NArchiveZoneAttr::NTelephone::AREA_CODE);
        if (telCodeAreaAttr != telephoneAttributes.end()) {
            area = WideToASCII(telCodeAreaAttr->second);
        }

        TString country;
        const TAttrConstIterator telCodeCountryAttr =
            telephoneAttributes.find(NArchiveZoneAttr::NTelephone::COUNTRY_CODE);
        if (telCodeCountryAttr != telephoneAttributes.end()) {
            country = WideToASCII(telCodeCountryAttr->second);
        }

        return TPhone(country, area, local);
    }

    void TUserTelephones::ProcessAttribute(const TUtf16String& attrName, const TUtf16String& attrValue, double idf)
    {
        TString attributeValue = WideToASCII(attrValue);
        if (!attributeValue.empty() && attributeValue[0] == '\"') {
            attributeValue = attributeValue.substr(1);
        }

        TPhoneInfo lastPhoneInfo;
        if (!Phones.empty()) {
            lastPhoneInfo = Phones.back();
            if (lastPhoneInfo.Type != Full) {
                Phones.pop_back();
            }
        }

        if (attrName == TEL_FULL) {
            if (lastPhoneInfo.Type == Local) {
                Phones.push_back(lastPhoneInfo);
            }
            lastPhoneInfo.Type = Full;
            lastPhoneInfo.Phone = TPhone(StringToPhone(attributeValue));
        } else if (attrName == TEL_CODE_AREA) {
            if (lastPhoneInfo.Type == Local) {
                lastPhoneInfo.Type = Full;
                lastPhoneInfo.Phone = TPhone(TString(), attributeValue, lastPhoneInfo.Phone.GetLocalPhone());
            } else {
                lastPhoneInfo.Type = CodeArea;
                lastPhoneInfo.Phone = TPhone(TString(), attributeValue, TString());
            }
        } else if (attrName == TEL_LOCAL) {
            switch (lastPhoneInfo.Type) {
                case None:
                    lastPhoneInfo.Type = Local;
                    break;
                case CodeArea:
                    lastPhoneInfo.Type = Full;
                    break;
                case Local:
                    Phones.push_back(lastPhoneInfo);
                    break;
                case Full:
                    lastPhoneInfo.Phone.SetAreaCode(TString());
                    lastPhoneInfo.Type = Local;
                    break;
                default:
                    break;
            }
            lastPhoneInfo.Phone = TPhone(TString(), lastPhoneInfo.Phone.GetAreaCode(), attributeValue);
        }

        if (lastPhoneInfo.Type != None) {
            lastPhoneInfo.Idf = Max(lastPhoneInfo.Idf, idf);
            Phones.push_back(lastPhoneInfo);
        }
    }

    const TVector<TUserTelephones::TPhoneInfo>& TUserTelephones::GetPhones() const
    {
        return Phones;
    }

}
