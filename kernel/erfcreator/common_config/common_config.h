#pragma once

#include <util/generic/string.h>

struct TErfCreateConfigCommon {
    bool Robot = false;
    bool FastTier = false;
    const char* RobotHome = nullptr;
    const char* SpiderHome = nullptr;
    int RobotCluster = 0;
    TString IndexDir;
    TString Index;
    TString Walrus;
    TString Archive;
    TString ArchiveSnippet;
    TString TagArchiveHeaders;
    TString ArcArchiveHeaders;
    TString Attrs;
    TString Catalog;
    TString ErfDataDir;
    TString Mirrors;
    TString Hosts;
    TString Urls;
    TString LinkIndex;
    TString XMap;
    TString Output;
    TString Pagerank;
    TString UkrainPageRank;
    TString Yanddata;
    TString SortedData;
    TString ErfInput;
    TString ErfFileName;
    TString HostErfInput;

    bool IgnoreErf = false;
    bool IgnoreLink = false;
    bool IgnoreLerf2 = false;
    bool IgnoreAttrs = false;
    bool IgnoreUrls = false;
    bool IgnoreArchives = false;
    bool IgnoreHashed = false;
    bool IgnoreQueries = false;
    bool IgnoreNevasca2 = false;
    bool IgnoreHits = false;
    bool CutMirrors = false;

    const char* Generate = nullptr;
    const char* UseGenerated = nullptr;
    int PrintBlock = 0;

    const char* GenerateUrl = nullptr;
    const char* UseGeneratedUrl = nullptr;

    TString HostFactorsPath;

    bool Patch = false;
    bool Patch2 = false;
    bool Patch2Erf2 = false;
    TString HC2N;
    TString PatchFields;

    bool UseLocalMirrors = false;
    bool UseMappedMirrors = false;

    bool Markers = false;

    bool PruningFloat = false;
    bool PruningInteger = false;

    bool UrlSequences = false;
    bool TitleSequences = false;

    bool HostErf = false;
    bool RegErf = false;
    bool RegHostErf = false;
    bool DocErf2 = false;
    bool GenerateAdultness;

    int NClusters = 0;
    TString TempPath = ".";
    TString SortedUrls;
    TString SortedUrlUids;
    TString SortedHosts;
    TString AntiSpam;
    TString Areas;

    const char* AdultnessStats = nullptr;

    TString PureFname;

    bool InvHashes = false;
    bool ReInvHashes = false;

    TString IndexSeg;

    bool UseHR = false;

    bool CreateAllAura = false;
    bool GenerateGsk = false;
    bool Attr2Erf = false;
    bool Attr2Erf2 = false;
    bool Version = false;

    void ParseArgs(int argc, char *argv[]);
};
