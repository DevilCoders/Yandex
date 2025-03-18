#include "localized_data.h"

#include <util/memory/blob.h>
#include <util/generic/vector.h>


namespace NAntiRobot {
    namespace NStatic {
        static const unsigned char captcha_data[] = {
            #include "captcha.inc"
        };

    }

    TLocalizedData::TLocalizedData()
        : ArchiveReader(TBlob::NoCopy(NStatic::captcha_data, sizeof(NStatic::captcha_data)))
    {
#include "captcha_runtime_variables.inc"
    }

    const TStringMap& TLocalizedData::GetData(TStringBuf lang) const {
        TString  key = TString("lang/") + lang;

        TMapOfStringMap::const_iterator it = Hash.find(key);
        if (it == Hash.end()) {
            key = TStringBuf("lang/ru");
            it = Hash.find(key);
            if (it == Hash.end()) {
                ythrow yexception() << "Could not find language in runtime variables hash";
            }
        }
        return it->second;
    }

    const TVector<TString>& TLocalizedData::GetExternalVersionedFiles() const {
        static TVector<TString> list = {
#include "external_static_versions.inc"
        };
        return list;
    }

    const TVector<int>& TLocalizedData::GetExternalVersionedFilesVersions() const {
        static TVector<int> versions = {
#include "external_static_version_list.inc"
        };
        return versions;
    }

    const TVector<TString>& TLocalizedData::GetAntirobotVersionedFiles() const {
        static TVector<TString> list = {
#include "antirobot_static_versions.inc"
        };
        return list;
    }

    const TVector<int>& TLocalizedData::GetAntirobotVersionedFilesVersions() const {
        static TVector<int> versions = {
#include "antirobot_static_version_list.inc"
        };
        return versions;
    }
} // namespace NAntiRobot
