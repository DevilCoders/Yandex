#include "shard_conf.h"

#include <library/cpp/digest/md5/md5.h>

#include <util/charset/unidata.h>
#include <util/generic/algorithm.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/string/strip.h>

TShardConfFileEntry::TShardConfFileEntry(const TString& name, const TString& md5Hex, time_t mTime, size_t size)
    : Name(name)
    , MD5(md5Hex)
    , MTime(mTime)
    , Size(size)
{
}

TString TShardConfFileEntry::ToString() const {
    if (!Name) {
        return TString();
    }

    TStringBuilder attr;
    attr << "%attr(";

    if (NoCheckMTime) {
        attr << "nocheckmtime, ";
    }

    if (NoCheckSum) {
        attr << "nochecksum, ";
    }

    if (NoCheckSize) {
        attr << "nochecksize, ";
    }

    if (NoCheck) {
        attr << "nocheck, ";
    }

    if (MTime && *MTime) {
        attr << "mtime=" << *MTime << ", ";
    }

    if (MD5) {
        attr << "md5=" << MD5 << ", ";
    }

    if (Size) {
        attr << "size=" << *Size << ", ";
    }

    if (attr.EndsWith(", ")) {
        attr.resize(attr.size() - 2);
    }

    attr << ") " << Name << "\n";
    return attr;
}

void FillNameAndAttr(const TStringBuf& line, TString& attrs, TString& name) {
    // "(some attr data)  some name data" ->
    //   (attrs="some attr data", name="some name data")
    TStringBuf processedLine = line;
    auto rightBracketPos = processedLine.find(')');
    Y_ENSURE(!processedLine.empty() && processedLine[0] == '(' && rightBracketPos != TStringBuf::npos,
             "Attribute data must be surrounded by () brackets");
    attrs = processedLine.substr(1, rightBracketPos - 1);
    processedLine.Skip(rightBracketPos + 1);
    auto startWhitespaces = processedLine.find_first_not_of("\t\n\v\f\r ");
    Y_ENSURE(!processedLine.empty() || startWhitespaces != TStringBuf::npos, "No name for attribute");
    Y_ENSURE(isspace(processedLine[0]),
             "Attribute data and name must be separated with space symbols (in line " << TString{line}.Quote() << ")");
    name = processedLine.Tail(startWhitespaces);
    Y_ENSURE(ConsistsOnlyGraphicalAndNonEmpty(name), "Name has unsupported characters in " << TString{line}.Quote());
}

TShardConfFileEntry TShardConfFileEntry::ParseFromString(TStringBuf line) {
    TShardConfFileEntry entry;

    line = StripString(line);

    if (line.SkipPrefix("%attr")) {
        TString attrs, name;
        FillNameAndAttr(line, attrs, name);

        entry.Name = name;

        if (attrs) {
            TVector<TString> tokens = SplitString(attrs, ",");
            for (TStringBuf token : tokens) {
                token = StripString(token);

                TStringBuf key, value;
                token.Split('=', key, value);

                key = StripString(key);
                value = StripString(value);

                if ("size" == key) {
                    entry.Size = ::FromString<size_t>(value);
                } else if ("md5" == key) {
                    entry.MD5 = value;
                } else if ("mtime" == key) {
                    entry.MTime = ::FromString<time_t>(value);
                } else if ("nocheckmtime" == key) {
                    entry.NoCheckMTime = true;
                } else if ("nochecksum" == key) {
                    entry.NoCheckSum = true;
                } else if ("nochecksize" == key) {
                    entry.NoCheckSize = true;
                } else if ("nocheck" == key) {
                    entry.NoCheck = true;
                }
            }
        }
    } else {
        Y_ENSURE(ConsistsOnlyGraphicalAndNonEmpty(line),
                 "Unsupported symbols in " << TString{line}.Quote());
        entry.Name = line;
        entry.NoCheckMTime = true;
        entry.NoCheckSum = true;
        entry.NoCheckSize = true;
    }

    return entry;
}

void TShardConfFileEntry::CheckFile(const TFsPath& directory) const {
    Y_ENSURE(!!Name, "empty name");
    TFsPath file = directory / Name;

    if (NoCheck) {
        return;
    }

    Y_ENSURE(file.Exists(), "does not exist: " << file.GetPath());
    Y_ENSURE(file.IsFile(), "not a file: " << file.GetPath());

    if (!NoCheckSize && Size) {
        TFileStat stat;
        Y_ENSURE(file.Stat(stat), "could not stat: " << file.GetPath());
        Y_ENSURE(stat.Size == *Size, "sizes differ (expected " << *Size << ", got " << stat.Size << "): " << file.GetPath());
    }

    if (!NoCheckSum && MD5) {
        TString md5 = GetMD5(file);
        Y_ENSURE(md5 == MD5, "md5 differ (expected " << MD5 << ", got " << md5 << "): " << file.GetPath());
    }
}

TShardConfFileEntry TShardConfFileEntry::InitFromFileStat(const TFsPath& file) {
    TShardConfFileEntry result;
    Y_ENSURE(file.Exists(), "does not exist: " << file.GetPath());
    Y_ENSURE(file.IsFile(), "not a file: " << file.GetPath());

    result.Name = file.GetName();

    TFileStat stat;
    Y_ENSURE(file.Stat(stat), "could not stat: " << file.GetPath());
    result.Size = stat.Size;
    result.MTime = stat.MTime;
    result.MD5 = GetMD5(file);
    return result;
}

TString TShardConfFileEntry::GetMD5(const TFsPath& file) {
    return ::MD5::MD5::File(file);
}


TShardName::TShardName(const TString& name, ui32 num, time_t tstamp)
    : Name(name)
    , Number(num)
    , Timestamp(tstamp)
{}

TString TShardName::ToString() const {
    return TStringBuilder() << Name << "-" << LeftPad(Number, 3, '0') << "-" << LeftPad(Timestamp, 10, '0');
}

TShardName TShardName::ParseFromString(const TStringBuf& data) {
    TString name;
    ui32 number = 0;
    ui32 tstamp = 0;
    TStringBuf processedData = data, tstampStrBuf, numberStrBuf;
    Y_ENSURE(processedData.RNextTok('-', tstampStrBuf) && processedData.RNextTok('-', numberStrBuf),
             "Can\'t separate number and timestamp in line " << TString{processedData}.Quote() << ". They must be separated with -");
    Y_ENSURE(tstampStrBuf.length() == 10 && numberStrBuf.length() == 3,
             "Timestamp must be in 10 digit format, number -- in 3 digit one");
    number = FromString<ui32>(numberStrBuf);
    tstamp = FromString<ui32>(tstampStrBuf);
    for (const char ch : processedData) {
        Y_ENSURE(isalpha(ch) || ch == '-', "Unsupported character in name " << TString{processedData}.Quote());
    }
    name = processedData;
    return TShardName(name, number, tstamp);
}


TShardConf::TShardConf(
        const TInstant timestamp,
        const TString& shardName,
        const TString& prevShardName,
        const TVector<TShardConfFileEntry>& files,
        const TVector<TString>& installScript
    )
    : Timestamp(timestamp)
    , ShardName(shardName)
    , PrevShardName(prevShardName)
    , Files(files)
    , InstallScript(installScript)
{
}


TShardConf TShardConf::ParseFromString(TStringBuf data) {
    TShardConf result;
    enum EState {
        S_MAIN, S_FILES, S_INSTALL
    } state = S_MAIN;

    while (data) {
        TStringBuf line = StripString(data.NextTok('\n'));
        line = StripString(line.NextTok('#'));

        if (!line) {
            continue;
        }

        if (line.StartsWith("%shard")) {
            state = S_MAIN;

            Y_ENSURE(!result.ShardName,
                   "duplicate instruction: " << line);

            TString shard = ReadAttribute<TString>(line, "%shard");
            Y_ENSURE(ConsistsOnlyGraphicalAndNonEmpty(shard),
                     "Non supported symbols in " << TString{line}.Quote());
            result.ShardName = shard;
        } else if (line.StartsWith("%mtime")) {
            state = S_MAIN;

            Y_ENSURE(!result.Timestamp.GetValue(),
                   "duplicate instruction: " << line);

            ui32 t = ReadAttribute<ui32>(line, "%mtime");
            result.Timestamp = TInstant::Seconds(t);
        } else if (line.StartsWith("%number")) {
            state = S_MAIN;

            Y_ENSURE(!result.ShardNumber,
                   "duplicate instruction: " << line);

            result.ShardNumber = ReadAttribute<ui32>(line, "%number");
        } else if (line.StartsWith("%requires")) {
            state = S_MAIN;

            Y_ENSURE(!result.PrevShardName,
                   "duplicate instruction: " << line);

            TString shard = ReadAttribute<TString>(line, "%requires");
            Y_ENSURE(ConsistsOnlyGraphicalAndNonEmpty(shard),
                     "Non supported symbols in " << TString{line}.Quote());
            result.PrevShardName = shard;
        } else if (line == "%files") {
            state = S_FILES;
        } else if (line.StartsWith("%attr")) {
            state = S_FILES;
            result.Files.push_back(TShardConfFileEntry::ParseFromString(line));
        } else if (line.StartsWith("%install_size")) {
            state = S_MAIN;

            Y_ENSURE(!result.InstallSize,
                   "duplicate instruction: " << line);

            result.InstallSize = ReadAttribute<ui64>(line, "%install_size");
        } else if (line == "%install") {
            state = S_INSTALL;
        } else if (line.StartsWith('%')) {
            state = S_MAIN;
        } else if (state == S_INSTALL) {
            result.InstallScript.push_back(TString{line});
        } else if (state == S_FILES) {
            result.Files.push_back(TShardConfFileEntry::ParseFromString(line));
        }
    }

    return result;
}

TShardConf TShardConf::ParseFromFile(const TFsPath& file) {
    return ParseFromString(TFileInput(file).ReadAll());
}

void TShardConf::CheckShard(const TFsPath& shardDir) const {
    for (const auto& file : Files) {
        file.CheckFile(shardDir);
    }
}

TString TShardConf::ToString() const {
    TStringBuilder data;

    if (ShardName) {
        data << "%shard " << ShardName << "\n";
    }

    if (Timestamp.Seconds()) {
        data << "%mtime " << Timestamp.Seconds() << "\n";
    }

    if (ShardNumber) {
        data << "%number " << *ShardNumber << "\n";
    }

    if (PrevShardName) {
        data << "%requires " << PrevShardName << "\n";
    }

    data << "%files\n";

    for (const auto& file : Files) {
        data << file.ToString();
    }

    if (InstallSize) {
        data << "%install_size " << *InstallSize << "\n";
    }

    if (PrevShardName && InstallScript) {
        data << "%install\n";
        for (const auto& line : InstallScript) {
            data << line << "\n";
        }
        data << "\n";
    }

    return data;
}


TShardConfWriter::TShardConfWriter(const TShardConf& shardConf)
    : Conf(shardConf)
{
}

TShardConfWriter::TShardConfWriter(
    const TInstant timestamp,
    const TString& shardName,
    const TString& prevShardName,
    const TVector<TShardConfFileEntry>& files,
    const TVector<TString>& installScript
)
    : TShardConfWriter(TShardConf(timestamp, shardName, prevShardName, files, installScript))
{}

void TShardConfWriter::Write(const TFsPath& outDir, const TString& fname) const {
    TFsPath shardConfPath = outDir.Child(fname);
    {
        TOFStream file(shardConfPath);
        file << Conf.ToString();
        file.Finish();
    }
}
