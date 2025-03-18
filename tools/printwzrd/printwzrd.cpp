#include "lib/options.h"
#include "lib/printer.h"
#include "lib/defaultcfg.h"
#include "lib/verify_gazetteer.h"
#include "process_print.h"

#include <library/cpp/eventlog/eventlog.h>
#include <ysite/yandex/reqdata/reqdata.h>
#include <search/wizard/wizard.h>
#include <search/wizard/config/config.h>
#include <search/wizard/core/data_loader.h>
#include <search/wizard/core/wizglue.h>
#include <search/wizard/remote/remote_wizard.h>

#include <util/folder/dirut.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

#include <util/stream/output.h>

static inline TAutoPtr<TWizardConfig> CreatePrinwzrdConfig(const TPrintwzrdOptions& options) {
    TAutoPtr<TWizardConfig> ret;
    if (!options.RulesList.empty())
        ret = new TDefaultPrintwzrdCfg(options.WorkDir, options.CustomRuntimeData, options.RulesList);
    else {
        ret = TWizardConfig::CreateWizardConfig(options.PathToConfig.data(), options.WorkDir.data());
        if (options.CustomRuntimeData) {
            ret->SetReqWizardRuntimeDir(options.CustomRuntimeData);
            if (TFsPath(options.CustomRuntimeData).IsSymlink())
                ret->SetReqWizardRealtimeDirReal(TFsPath(options.CustomRuntimeData).ReadLink());
            else
                ret->SetReqWizardRuntimeDirReal(options.CustomRuntimeData);
        }
        if (options.CustomRealtimeData) {
            ret->SetReqWizardRealtimeDir(options.CustomRealtimeData);
            if (TFsPath(options.CustomRealtimeData).IsSymlink())
                ret->SetReqWizardRealtimeDirReal(TFsPath(options.CustomRealtimeData).ReadLink());
            else
                ret->SetReqWizardRealtimeDirReal(options.CustomRealtimeData);
        }
    }

    if (options.NoCache)
        ret->TurnOffCache();

    if (options.CollectStat)
        ret->SetCollectingRuntimeStat(true);

    ret->SetWizardThreadCount(options.ThreadCount);

    return ret;
}

static TAutoPtr<TReqWizard> CreateRequestWizard(const TPrintwzrdOptions& options, TEventLogPtr eventLog)
{
    TAutoPtr<TWizardConfig> cfg = CreatePrinwzrdConfig(options);

    NDataLoader::SetDataLoaderMode(options.FastLoader);

    if (options.VerifyGazetteer)
        NPrintWzrd::VerifyGazetteerBinaries(*cfg);

    return CreateRequestWizard(*cfg, eventLog);
}

int main(int argc, char *argv[])
{
    using namespace NPrintWzrd;
    TPrintwzrdOptions options;
    options.Reset(argc, argv);

    Y_VERIFY(options.OkLocal() || options.OkRemote(), "Missing required options");

    TEventLogPtr eventLog;
    if (options.EventLogPath) {
        TString path = options.EventLogPath;
        eventLog = new TEventLog(path, NEvClass::Factory()->CurrentFormat());
        eventLog->ReopenLog();
    } else {
        eventLog = new TEventLog(0);
    }

    try {
        THolder<TReqWizard> localWizard(options.OkLocal() ? CreateRequestWizard(options, eventLog).Release() : nullptr);
        THolder<TRemoteWizard> remoteWizard(options.OkRemote() ? CreateTestRemoteWizard(options.Port, options.Host) : nullptr);

        if (options.CompareReqRemote) {
            ProcessCompareTest(localWizard.Get(), remoteWizard.Get(), options, eventLog);
        } else {
            IRequestWizard* wizard = localWizard.Get();
            if (!wizard)
                wizard = remoteWizard.Get();
            Y_VERIFY(wizard, "IRequestWizard is not created");

            ProcessSingleTest(wizard, options, eventLog);
            if (options.CollectStat && options.OkLocal()) {
                TStringStream cacheStr;
                localWizard->ReportCacheStats(cacheStr);
                Cout << "CacheStats" << cacheStr.Str() << Endl;
            }
        }
    } catch (const std::exception& e) {
        Cerr << "printwzrd: " << e.what() << Endl;
        return 1;
    }
    return 0;
}
