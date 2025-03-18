#pragma once
#include <util/generic/string.h>
#include <library/cpp/getopt/small/last_getopt.h>
#include <mapreduce/yt/interface/common.h>

class TYtTokenCommandLineOption {
private:
    TString Token;
    TString ClientId;
    TString ClientSecret;
public:
    TYtTokenCommandLineOption(
        NLastGetopt::TOpts& options,
        const TStringBuf& clientId,
        const TStringBuf& clientSecret
    );
    TString Get();
};

class TYtProxyCommandLineOption {
private:
    TString Proxy;
public:
    TYtProxyCommandLineOption(NLastGetopt::TOpts& options);
    TString Get();
};

class TJobCountConfigCommandLineOptions {
private:
    int JobCount;
    int RecordPerJobCount;
public:
    TJobCountConfigCommandLineOptions(NLastGetopt::TOpts& options);
    i64 Get(const TString& inputTable, NYT::IClientPtr& client);
    bool Defined() { return JobCount > 0 || RecordPerJobCount > 0; }
};

struct TDataFile {
    TString CypressPath;
    TString LocalName;
};

struct TDataFilesList {
    TVector<TDataFile> Files;
    ui64 TotalSize;
};

class TDataFilesCommandLineOptions {
private:
    TString LocalPath;
    TString CypressPath;
    TString CypressCachePath;
    int ExpirationDays;
    TString CypressPathsFile;
public:
    TDataFilesCommandLineOptions(const TString& name, NLastGetopt::TOpts& options);
    TDataFilesList Get(NYT::IClientPtr& client, const TVector<TString>& selectedRules);
    bool Defined() {
        return !CypressPath.Empty() || !CypressPathsFile.Empty() ||
               (!CypressCachePath.Empty() && !LocalPath.Empty());
    }
private:
    TVector<TString> UploadLocalFiles(NYT::IClientPtr& client, const TVector<TString>& selectedRules);
    TVector<TString> GetCypressFiles(NYT::IClientPtr& client);
};

struct TDirectModeColumns {
    TString QueryColumn;
    TString RegionColumnOrValue;
    bool RegionIsValue;
};

class TDirectModeCommandLineOptions {
private:
    TString DirectMode;
public:
    TDirectModeCommandLineOptions(NLastGetopt::TOpts& options);
    TDirectModeColumns Get(const TString& inputTable, NYT::IClientPtr& client);
    bool Defined() { return !DirectMode.Empty(); }
};
