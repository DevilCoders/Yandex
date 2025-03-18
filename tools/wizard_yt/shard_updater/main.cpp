#include <tools/wizard_yt/shard_packer/shard_packer.h>
#include <tools/wizard_yt/reducer_common/rule_updater.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/string/join.h>
#include <util/string/cast.h>
#include <util/thread/pool.h>
#include <util/system/mutex.h>
#include <util/system/env.h>
#include <util/stream/hex.h>

using namespace NYT;

int main(const int argc, const char *argv[]) {
    Initialize(argc, argv);
    NLastGetopt::TOpts options;
    TRuleUpdaterContext ctx;

    TString localPath;
    int threads;
    TString resultPathsFile;

    options
        .AddLongOption('d', "data", "path to local shard")
        .Required()
        .RequiredArgument("DATA")
        .StoreResult(&localPath);

    options
        .AddLongOption('o', "output", "output shard path in Cypress")
        .RequiredArgument("OUTPUT")
        .StoreResultT<TString>(&ctx.ShardPath);

    options
        .AddLongOption('c', "cache-path", "path to cache in Cypress")
        .Required()
        .RequiredArgument("CACHE")
        .StoreResult(&ctx.CachePath);

    options
        .AddLongOption("ttl", "Cypress ttl for shard files")
        .RequiredArgument("TTL")
        .Required()
        .StoreResult(&ctx.ExpirationTime);

    options
        .AddLongOption('j', "jobs", "Number of threads")
        .Optional()
        .DefaultValue(32)
        .StoreResult(&threads);

    options
        .AddLongOption("debug", "Print debug info")
        .NoArgument()
        .SetFlag(&ctx.EnableDebug);

    options
        .AddLongOption("result-paths-file", "File with cypress output paths")
        .Optional()
        .RequiredArgument("RESULT_PATHS_FILE")
        .StoreResult(&resultPathsFile);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    auto client = NYT::CreateClient(GetEnv("YT_PROXY"));

    TFsPath shard(localPath);
    TVector<TFsPath> children;
    shard.List(children);

    TThreadPool q(TThreadPool::TParams().SetBlocking(true).SetCatching(false));
    q.Start(threads);

    TVector<TString> resultPaths;
    client->Create(ctx.CachePath, NYT::ENodeType::NT_MAP, TCreateOptions().IgnoreExisting(true).Recursive(true));
    if (!ctx.ShardPath.Empty()) {
        client->Remove(ctx.ShardPath, TRemoveOptions().Recursive(true).Force(true));
    }
    for (auto& ruleDir : children) {
        if (ruleDir.IsDirectory()) {
            q.SafeAddAndOwn(THolder(new TRuleDirUpdater(ruleDir, client, ctx, resultPaths)));
        }
    }
    q.Stop();
    if (ctx.Failed) {
        Cerr << "Failed to update shard" << Endl;
        return 1;
    }

    if (!resultPathsFile.empty()) {
        TFileOutput file(resultPathsFile);
        for (auto resultPath : resultPaths) {
            file.Write(resultPath + "\n");
        }
        file.Finish();
    }
    return 0;
}
