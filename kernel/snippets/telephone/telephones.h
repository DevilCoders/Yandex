#pragma once

#include <library/cpp/telfinder/phone.h>

#include <util/generic/string.h>
#include <util/generic/hash.h>


namespace NSnippets
{
    class TUserTelephones
    {
    public:
        //! Allowed phone types
        enum PhoneType {
            Full,
            Local,
            CodeArea,
            None,
        };

        struct TPhoneInfo {
            TPhone Phone;
            PhoneType Type;
            double Idf;

            TPhoneInfo()
                : Type(None)
                , Idf(0)
            {
            }
        };

    private:
        static const TUtf16String TEL_LOCAL;
        static const TUtf16String TEL_CODE_AREA;
        static const TUtf16String TEL_FULL;

    private:
        TVector<TPhoneInfo> Phones;

    private:
        static TPhone StringToPhone(const TString& phoneRepresentation);
        static TPhone StringToPhone(const TUtf16String& phoneRepresentation);

    public:
        //! Convert archive zone attributes to phone
        static TPhone AttrsToPhone(const THashMap<TString, TUtf16String>& telephoneAttributes);

        //! Check attribute name
        static bool IsPhoneAttribute(const TUtf16String& attributeName);

    public:
        //! Process incoming attribute
        void ProcessAttribute(const TUtf16String& attrName, const TUtf16String& attrValue, double idf);

        //! Get saved phones
        const TVector<TPhoneInfo>& GetPhones() const;
    };
}
