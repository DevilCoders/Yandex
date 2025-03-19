#include "common_config.h"

#include "paths.h"

#include <library/cpp/getopt/opt2.h>

#include <util/string/cast.h>

void TErfCreateConfigCommon::ParseArgs(int argc, char* argv[]) {
    Opt2 opt(argc, argv, "1l2aACeGhHILMnyp45:6789PqrQstvVwWxzZOb:3:B:c:d:D:E:f:F:g:i:j:k:K:m:N:o:R:S:T:u:U:X:Y:J:", 0,
            "pure=Y,generate-indexregherf=z,erf-name=3");

    CutMirrors       = opt.Has('1', "cut mirrors");
    IgnoreErf        = opt.Has('e', "- don't generate indexerf");
    IgnoreLink       = opt.Has('l', "- don't generate indexlerf index.reflerf");
    IgnoreLerf2      = opt.Has('2', "- don't generate 'new link quality' feature");
    IgnoreAttrs      = opt.Has('A', "- don't generate features from attrs");
    IgnoreArchives   = opt.Has('a', "- don't generate features from archives");
    IgnoreUrls       = opt.Has('L', "- don't generate features from urls");
    IgnoreHashed     = opt.Has('h', "- don't generate features from catalog");
    IgnoreQueries    = opt.Has('q', "- don't generate features from queries");
    IgnoreNevasca2   = opt.Has('V', "- ignore nevasca2 data");
    IgnoreHits       = opt.Has('H', "- ignore hits data");
    Markers          = opt.Has('r', "- generate markers");
    PruningFloat     = opt.Has('P', "- generate float pruning rank");
    PruningInteger   = opt.Has('w', "- generate integer pruning rank");
    UrlSequences     = opt.Has('s', "- generate url sequences array");
    TitleSequences   = opt.Has('t', "- generate title sequences array");
    HostErf          = opt.Has('x', "- generate host erf");
    RegErf           = opt.Has('C', "- generate regional erf (indexregerf)");
    RegHostErf       = opt.Has('z', "- generate regional host erf (indexregherf)");
    DocErf2          = opt.Has('O', "- generate doc erf2");
    FastTier         = opt.Has('Z', "- FastTier mode");

    UseHR            = opt.Has('W', "- use HR file for HostRank factor");

    RobotHome        = opt.Arg('R', "<dir> - mode for work on walrus, path to (/Berkanavt/database)", nullptr);
    SpiderHome       = opt.Arg('B', "<dir> - path to (/Berkanavt/dbspider)", "/Berkanavt/dbspider");
    RobotCluster     = opt.Int('k', "<num> - cluster index (for -R)", -1);
    Catalog          = opt.Arg('c', "<dir> - catalog dir (for -R)", "/Berkanavt/catalog");
    Mirrors          = opt.Arg('d', "<dir> - mirrors dir (for -R)", "/Berkanavt/bin/data");
    Output           = opt.Arg('o', "<dir> - output dir", ".");
    Output += "/";

    Generate         = opt.Arg('g', "<out:catalog.bin> - generate data", nullptr);
    UseGenerated     = opt.Arg('D', "<in:catalog.bin> - use generated data", nullptr);
    PrintBlock       = opt.Int('i', "<index in catalog bin> - to print block", -1, 0);
    GenerateUrl      = opt.Arg('u', "<out:urls.bin> - generate url data", nullptr);
    UseGeneratedUrl  = opt.Arg('U', "<in:urls.bin> - use generated url data", nullptr);
    HostFactorsPath  = opt.Arg('F', "path to hostfactors.*", nullptr);
    UseMappedMirrors = !opt.Has('M', "- don't use mapped mirrors");
    NClusters        = opt.Int('N', "- number of clusters", 12);
    PatchFields      = opt.Arg('f', "[!]<field>,<field> - patch fields", "!");
    ErfDataDir       = opt.Arg('E', "<dir> - erfdata dir", "/Berkanavt/erfdata");
    Patch            = opt.Has('p', "- enable patch mode");
    Patch2           = opt.Has('4', "- enable patch2 mode");
    Patch2Erf2       = opt.Has('6', "- enable patch2 mode for erf2");
    CreateAllAura    = opt.Has('7', "- generate all aura");
    GenerateGsk      = opt.Has('8', "- generate gsk");
    GenerateAdultness = opt.Has('9', "- generate adultness");
    HC2N             = opt.Arg('5', "- h.c2n file for patch2 on base searches", nullptr);
    PureFname        = opt.Arg('Y', "- pure.trie filename", "");
    InvHashes        = opt.Has('I', "- generate inverted hashes");
    ReInvHashes      = opt.Has('Q', "- regenerate inverted hashes");
    IndexSeg         = opt.Arg('K', "- generate segment index", "");
    ErfInput         = opt.Arg('b', "- full path to indexerf/indexerf2 for reading", Output + "indexerf");
    ErfFileName      = opt.Arg('3', "- index erf file name", "indexerf");
    HostErfInput     = opt.Arg('J', "- full path to indexherf for reading", Output + "indexherf");

    IndexDir = opt.Arg('j', "- full path to index directory", ".");
    TempPath = opt.Arg('T', "- temporary path", Sprintf("%s/walrus/%03d/tmp", RobotHome, RobotCluster));
    AntiSpam = opt.Arg('S', "- antispam path", "/Berkanavt/antispam");
    Areas    = opt.Arg('j', "- path to areas.lst (/Berkanavt/urlrules/areas.lst by default)", "/Berkanavt/urlrules/areas.lst");
    AdultnessStats = opt.Arg('X', "<out:host_adultness.txt> - gather host adultness stats", nullptr);

    Attr2Erf        = opt.Has('n', "- expand erf with HostId and DomainId attributes from attrs");
    Attr2Erf2       = opt.Has('y', "- expand erf2 with HostId and DomainId attributes from attrs");
    Version         = opt.Has('v', "- print version");

    opt.AutoUsageErr("");

    if (Patch) {
        IgnoreLink = true;
        IgnoreLerf2 = true;
        IgnoreAttrs = true;
        IgnoreArchives = true;
    }

    Robot = RobotHome != nullptr;
    if (Robot) {
        Y_VERIFY(FastTier == false, "Only one mode can be active - either FastTier or Robot!");
        Hosts = TString::Join(RobotHome, "/walrus");
        Yanddata = Sprintf("%s/yanddata", SpiderHome);
        SortedData = TString::Join(RobotHome, "/walrus");

        if (!Generate && !GenerateUrl && !HostErf && !RegHostErf && !CreateAllAura && !CutMirrors) {
            if (-1 != RobotCluster) {
            Index = Sprintf("%s/yandex/oldbd%03d", RobotHome, RobotCluster);
            Archive = Index;
            ArchiveSnippet = Archive + "arc";
            Walrus = Sprintf("%s/walrus/%03d", RobotHome, RobotCluster);
            Attrs = Walrus;
            Urls  = Walrus + "/url.dat";
            XMap = Sprintf("%s/google/yandex/ref%03d", RobotHome, RobotCluster);
            LinkIndex = Sprintf("%s/google/yandex/goo%03dc", RobotHome, RobotCluster);
            Pagerank = Sprintf("%s/pagerank/PRs.%03d", RobotHome, RobotCluster);
            UkrainPageRank = Sprintf("%s/pagerank/PRsua.%03d", RobotHome, RobotCluster);
            TagArchiveHeaders = TagHeadersDatPath(RobotHome, RobotCluster);
            ArcArchiveHeaders = ArcHeadersDatPath(RobotHome, RobotCluster);
            } else {
                if (!Markers && !PruningFloat)
                    ythrow yexception() << "robot cluster should be specified";
            }
        }
    } else {
        Index = IndexDir + "/index";
        Archive= IndexDir + "/index";
        ArchiveSnippet = Archive;
        Walrus = IndexDir;
        Attrs = IndexDir;
        Catalog = IndexDir;
        Mirrors = IndexDir;
        Hosts = IndexDir;
        SortedData = IndexDir;
        Urls = IndexDir + "/url.dat";
        XMap = IndexDir + "/index";
        LinkIndex = IndexDir + "/index";
        Pagerank = IndexDir + "/indexpr";
        UkrainPageRank = IndexDir + "/indexprua";
    }

    SortedUrls = Sprintf("%s/sorted_urls.dat", TempPath.data());
    SortedUrlUids = Sprintf("%s/sorted_urluids.dat", TempPath.data());
    SortedHosts = Sprintf("%s/sorted_hosts.dat", TempPath.data());
}

