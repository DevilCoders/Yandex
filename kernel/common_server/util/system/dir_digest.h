#pragma once

#include <util/generic/map.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <library/cpp/regex/pcre/regexp.h>
#include <util/generic/ptr.h>
#include <util/system/mutex.h>
#include <util/folder/path.h>
#include <util/system/file.h>
#include <library/cpp/json/writer/json_value.h>

struct TFileHashInfo {
    TString Hash;
    ui64 Size;
};

typedef THashMap<TString, TFileHashInfo> TDirHashInfo;

TDirHashInfo GetDirHashes(const TString& path, const TRegExMatch* filter = nullptr, const TRegExMatch* exclude = nullptr);

class TFilesSizeInfo {
private:
    TMap<TString, ui64> Sizes;
    TMap<TString, ui64> Count;
    TMutex Mutex;
    ui32 PrefixSize = 25;
public:

    using TPtr = TAtomicSharedPtr<TFilesSizeInfo>;

    TFilesSizeInfo() {
    }

    TFilesSizeInfo(const TString& filePath, ui32 prefixSize = 25) {
        PrefixSize = prefixSize;
        if (TFsPath(filePath).Exists()) {
            TVector<TFsPath> children;
            TFsPath(filePath).List(children);
            for (auto&& i : children) {
                TFileStat stat(i.GetPath());
                AddInfo(i.GetName(), stat.Size);
            }
        }
    }

    void AddInfo(const TString& fileName, ui64 size) {
        TGuard<TMutex> g(Mutex);
        Sizes[fileName.substr(0, PrefixSize)] += size;
        Count[fileName.substr(0, PrefixSize)]++;
    }

    void Merge(const TFilesSizeInfo& info) {
        TGuard<TMutex> g(Mutex);
        for (auto&& i : info.Sizes) {
            Sizes[i.first] += i.second;
        }
        for (auto&& i : info.Count) {
            Count[i.first] += i.second;
        }
    }

    NJson::TJsonValue GetReport() const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        TGuard<TMutex> g(Mutex);
        ui64 sumSize = 0;
        ui64 sumCount = 0;
        for (auto&& i : Sizes) {
            result.InsertValue(i.first, i.second);
            sumSize += i.second;
        }
        for (auto&& i : Count) {
            if (i.second > 1) {
                result.InsertValue(i.first + "_count", i.second);
                sumCount += i.second;
            }
        }
        result.InsertValue("__SUM", sumSize);
        result.InsertValue("__COUNT", sumCount);
        return result;
    }

};
