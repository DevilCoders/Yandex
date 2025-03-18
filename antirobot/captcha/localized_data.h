#pragma once

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/http/cookies/lctable.h>

#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>

namespace NAntiRobot {
    class TStringMap : public THashMap<TStringBuf, TStringBuf> {
    public:
        inline TStringBuf Get(const TStringBuf& key, TStringBuf def = TStringBuf()) const {
            const_iterator it = find(key);
            if (it == end()) {
                return def;
            }
            return it->second;
        }

        inline TStringBuf GetByService(const TStringBuf& key, const TString& suffix, TStringBuf def = TStringBuf()) const {
            const_iterator it = find(key + suffix);
            if (it == end()) {
                it = find(key);
                if (it == end()) {
                    return def;
                }
                return it->second;
            }
            return it->second;
        }
    };

    class TLocalizedData {
        typedef THashMap<TStringBuf, TStringMap, TStrBufHash, TStrBufEqualToCaseLess> TMapOfStringMap;

    private:
        TMapOfStringMap Hash;
        TArchiveReader ArchiveReader;

    public:
        TLocalizedData();

        const TStringMap& GetData(TStringBuf lang) const;

        inline const TArchiveReader& GetArchiveReader() const {
            return ArchiveReader;
        }

        static inline const TLocalizedData& Instance() {
            return *Singleton<TLocalizedData>();
        }

        const TVector<TString>& GetExternalVersionedFiles() const;
        const TVector<int>& GetExternalVersionedFilesVersions() const;
        const TVector<TString>& GetAntirobotVersionedFiles() const;
        const TVector<int>& GetAntirobotVersionedFilesVersions() const;
    };
}
