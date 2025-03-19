#pragma once

#include <util/datetime/base.h>
#include <util/folder/path.h>
#include <util/generic/maybe.h>
#include <util/string/vector.h>
#include <util/generic/algorithm.h>

struct TShardConfFileEntry {
    // %attr
    TString Name;
    TString MD5;           // md5=
    TMaybe<time_t> MTime; // mtime=
    TMaybe<size_t> Size;  // size=
    bool NoCheckMTime = false; // nocheckmtime
    bool NoCheckSum = false;   // nochecksum
    bool NoCheckSize = false;  // nochecksize
    bool NoCheck = false;      // nocheck

public:
    TShardConfFileEntry() = default;

    TShardConfFileEntry(const TString& name, const TString& md5Hex, time_t mTime, size_t size);

    bool operator< (const TShardConfFileEntry& other) const {
        return Name < other.Name;
    }

    TString ToString() const;

    void CheckFile(const TFsPath& directory) const;

    static TShardConfFileEntry ParseFromString(TStringBuf data);

    static TShardConfFileEntry InitFromFileStat(const TFsPath& file);

    static TString GetMD5(const TFsPath& file);
};


struct TShardName {
    TString Name;
    ui32 Number = 0;
    time_t Timestamp = 0;

public:
    TShardName() = default;

    TShardName(const TString& name, ui32 num, time_t tstamp);

    TString ToString() const;

    static TShardName ParseFromString(const TStringBuf& processedData);
};


struct TShardConf {
    TInstant Timestamp;    // %mtime
    TString ShardName;      // %shard
    TString PrevShardName;  // %requires
    TMaybe<ui32> ShardNumber;  // %number
    TVector<TShardConfFileEntry> Files; // %files
    TMaybe<ui64> InstallSize; // %install_size
    TVector<TString> InstallScript;  // %install

public:
    TShardConf() = default;

    TShardConf(const TInstant timestamp,
        const TString& shardName,
        const TString& prevShardName,
        const TVector<TShardConfFileEntry>& files,
        const TVector<TString>& installScript
    );

    TString ToString() const;

    void CheckShard(const TFsPath& shardDir) const;

    TShardName ParseShardName() const {
        return TShardName::ParseFromString(ShardName);
    }

    static TShardConf ParseFromString(TStringBuf data);

    static TShardConf ParseFromFile(const TFsPath& file);
};


class TShardConfWriter {
private:
    const TShardConf Conf;

public:
    TShardConfWriter(const TShardConf& shardConf);

    TShardConfWriter(
        const TInstant timestamp,
        const TString& shardName,
        const TString& prevShardName,
        const TVector<TShardConfFileEntry>& files,
        const TVector<TString>& installScript = TVector<TString>{"scripts/upload-search.sh"}
    );

    void Write(const TFsPath& outDir, const TString& fname = "shard.conf") const;
};

inline bool ConsistsOnlyGraphicalAndNonEmpty(const TStringBuf& str) {
    static const TString graphSymbols = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~]";
    if (str.empty()) {
        return false;
    }
    for (const char ch : str) {
        if (!(isalpha(ch) || isdigit(ch) || graphSymbols.Contains(ch))) {
            return false;
        }
    }
    return true;
}

inline void FillNameAndAttr(const TStringBuf& line, TString& name, TString& attrs);

template < typename T >
inline T ReadAttribute(TStringBuf str, const TStringBuf& attrName) {
    str.SkipPrefix(attrName);
    while (!str.empty() && str[0] == ' ') {
        str.Skip(1);
    }
    Y_ENSURE(!str.empty(), "No value for attribute \"" << attrName << "\"");
    return FromString<T>(str);
}
