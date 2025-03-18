#include "options.h"

#include "tools/mstand/squeeze_lib/lib/squeeze_impl.h"
#include "tools/mstand/squeeze_lib/lib/squeezer.h"

#include <yweb/blender/common/sessions_from_stream.h>

#include <mapreduce/yt/common/config.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/util/ypath_join.h>

#include <util/generic/size_literals.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/ysaveload.h>

/*
Build:
ya make squeeze_lib/bin --checkout && strip squeeze_lib/bin/bin

Local run:
yt read "//user_sessions/pub/search/daily/2021-08-01/clean[y203767431618655603:y2037674331523833729]" --format '<has_subkey=true>yamr' > user_sessions_example
./squeeze_lib/bin/bin --user-sessions user_sessions_example --mode local

Yt run:
./squeeze_lib/bin/bin --date 20210801 --lower-key y203767431618655603 --upper-key y203767431618655604 --dst-folder //tmp/ivankun --mode yt
*/

TString GetTempTablePath(const TString& dstFolder, const NMstand::TExperimentForSqueeze& exp) {
    auto tableName = TString::Join("mstand_temp_squeeze_", exp.Service, "_", exp.FilterHash, "_", exp.Testid, exp.Day.ToStroka("_%Y%m%d_"), CreateGuidAsString());
    return NYT::JoinYPaths(dstFolder, tableName);
}

TVector<NMstand::TExperimentForSqueeze> GetTestExperiments(const NSqueezeRunner::TCliOpts& opts) {
    TVector<NMstand::TExperimentForSqueeze> experiments;
    experiments.emplace_back(opts.Date, TVector<NMstand::TFilter>(), "", "web", "", "104036", 0, false);
    experiments.emplace_back(opts.Date, TVector<NMstand::TFilter>(), "", "web", "", "126313", 0, false);
    experiments.emplace_back(opts.Date, TVector<NMstand::TFilter>(), "", "web", "", "315735", 0, false);

    experiments[2].FilterHash = "100500";
    experiments[2].Filters.emplace_back("uitype_filter", "touch");

    for(ui32 i = 0; i < experiments.size(); ++i) {
        experiments[i].TableIndex = i;
        experiments[i].TempTablePath = GetTempTablePath(opts.DstFolder, experiments[i]);
    }
    return experiments;
}

TVector<NMstand::TExperimentForSqueeze> GetExperiments(const NSqueezeRunner::TCliOpts& opts) {
    if (opts.Services.size() == 0 || opts.Testids.size() == 0) {
        return GetTestExperiments(opts);
    }

    TVector<NMstand::TExperimentForSqueeze> experiments;
    for (const auto& service : opts.Services) {
        for (const auto& testid : opts.Testids) {
            experiments.emplace_back(opts.Date, TVector<NMstand::TFilter>(), "", service, "", testid, 0, false);
        }
    }
    for(ui32 i = 0; i < experiments.size(); ++i) {
        experiments[i].TableIndex = i;
        experiments[i].TempTablePath = GetTempTablePath(opts.DstFolder, experiments[i]);
    }
    return experiments;
}

NMstand::TYtParams GetYtParams(const NSqueezeRunner::TCliOpts& opts) {
    NMstand::TYtParams ytParams;
    ytParams.Pool = opts.Pool;
    ytParams.Server = opts.Server;
    ytParams.LowerKey = opts.LowerKey;
    ytParams.UpperKey = opts.UpperKey;
    ytParams.MemoryLimit = opts.MemoryLimitGB * 1_GB;
    ytParams.DataSizePerJob = opts.DataSizePerJobGB * 1_GB;
    ytParams.MaxDataSizePerJob = opts.MaxDataSizePerJobGB * 1_GB;

    ytParams.YtFiles.push_back("//home/mstand/resources/blockstat.dict");
    ytParams.YtFiles.push_back("//home/mstand/resources/browser.xml");
    ytParams.YtFiles.push_back("//home/mstand/resources/geodata4.bin");

    if (opts.Services.size() == 1 && opts.Services[0] == "market-search-sessions") {
        ytParams.SourcePaths.push_back(NYT::JoinYPaths("//user_sessions/pub/market/daily", opts.Date.ToStroka("%Y-%m-%d"), "clean"));
    }
    else {
        ytParams.SourcePaths.push_back(NYT::JoinYPaths("//statbox/cube/daily/event_money/v2.1", opts.Date.ToStroka("%Y-%m-%d")));
        ytParams.SourcePaths.push_back(NYT::JoinYPaths("//user_sessions/pub/direct_urls/daily", opts.Date.ToStroka("%Y-%m-%d"), "clean"));
        ytParams.SourcePaths.push_back(NYT::JoinYPaths("//user_sessions/pub/search/daily", opts.Date.ToStroka("%Y-%m-%d"), "clean"));
        ytParams.SourcePaths.push_back(NYT::JoinYPaths("//user_sessions/pub/search/daily", opts.Date.ToStroka("%Y-%m-%d"), "tech"));
        ytParams.SourcePaths.push_back(NYT::JoinYPaths("//user_sessions/pub/video/daily", opts.Date.ToStroka("%Y-%m-%d"), "clean"));
    }

    ytParams.YuidPaths.push_back(NYT::JoinYPaths("//home/abt/yuid_testids", opts.Date.ToStroka("%Y%m%d")));

    return ytParams;
}

void SqueezeLocal(const NSqueezeRunner::TCliOpts& opts) {
    TVector<NMstand::TExperimentForSqueeze> experiments;
    NMstand::TYtParams ytParams;
    if (opts.SqueezeParamsFile.empty()) {
        experiments = GetExperiments(opts);
        ytParams = GetYtParams(opts);
    }
    else {
        NMstand::TFilterMap filters;
        NMstand::LoadSqueezeParameters(opts.SqueezeParamsFile, experiments, ytParams, filters);
    }

    TBlockStatInfo blockstat(opts.BlockstatFile);
    TVector<NRA::TRequestsContainer> containers = NBlender::ParseFromFile(opts.UserSessionsFile, blockstat);

    NMstand::TUserSessionsSqueezer squeezer(experiments);
    squeezer.Init();
    for(const auto& container : containers)
    {
        auto result = squeezer.SqueezeSessions(container, TVector<NYT::TNode>(), THashMap<TString, NRA::TFilterPack>(), THashSet<TString>());
        Cerr << result.size() << Endl;
    }
}

void SqueezeYt(const NSqueezeRunner::TCliOpts& opts) {
    TVector<NMstand::TExperimentForSqueeze> experiments;
    NMstand::TYtParams ytParams;
    NMstand::TFilterMap filters;
    if (opts.SqueezeParamsFile.empty()) {
        experiments = GetExperiments(opts);
        ytParams = GetYtParams(opts);
    }
    else {
        NMstand::LoadSqueezeParameters(opts.SqueezeParamsFile, experiments, ytParams, filters);
    }
    auto operationId = NMstand::SqueezeDay(experiments, ytParams, filters);
    Cerr << "operation_id: " << operationId << Endl;

    if (!opts.OutputFile.empty()) {
        NMstand::SaveOuputParameters(opts.OutputFile, operationId);
    }
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    NYT::SetLogger(NYT::CreateStdErrLogger(NYT::ILogger::INFO));

    auto opts = NSqueezeRunner::ParseOptions(argc, argv);
    if (opts.RunMode == NSqueezeRunner::ESqueezeRunMode::LOCAL) {
        SqueezeLocal(opts);
    }
    else {
        SqueezeYt(opts);
    }

    return 0;
}
