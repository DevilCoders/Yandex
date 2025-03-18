#pragma once

#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/ysaveload.h>

struct TPrintwzrdOptions {
    TString WorkDir;
    TString CustomRuntimeData;
    TString CustomRealtimeData;
    TString PathToConfig;
    TString EventLogPath;

    bool FastLoader;    // use mem-mapped blobs for reading big data files if true
    bool PrintDolbilka;
    bool PrintExtraOutput;
    bool PrintLemmaCount;
    bool PrintQtree;
    bool PrintSuccessfulRules;
    bool PrintRichTree;
    bool PrintCgiParamRelev;
    bool TabbedInput;
    bool VerifyGazetteer;
    bool CompareReqRemote;
    bool NoCache;
    bool SortOutput;

    bool PrintMarkup;

    ui16 Port;
    TString Host;
    bool CgiInput;

    TString LocalWizardOutFile;
    TString RemoteWizardOutFile;

    TString AppendCgi;

    TString RulesList;
    TString RulesToPrint;

    TString InputFileName;
    TString OutputFileName;

    bool IsTestRun = false; // signal that printwzrd was started by automated tests
    bool DebugMode;
    bool CollectStat;
    bool PrintSrc;

    size_t ThreadCount;

    TPrintwzrdOptions();
    void Reset(int argc, char *argv[]);

    // enough options to run wizard in local mode
    bool OkLocal() const {
        return !WorkDir.empty() && (!PathToConfig.empty() || !RulesList.empty());
    }

    // enough options to run wizard in remote mode
    bool OkRemote() const {
        return !Host.empty() && Port != 0;
    }

    void Save(IOutputStream* stream) const {
        ::Save(stream, WorkDir);
        ::Save(stream, CustomRuntimeData);
        ::Save(stream, PathToConfig);
        ::Save(stream, EventLogPath);

        ::Save(stream, FastLoader);
        ::Save(stream, PrintDolbilka);
        ::Save(stream, PrintExtraOutput);
        ::Save(stream, PrintLemmaCount);
        ::Save(stream, PrintQtree);
        ::Save(stream, PrintSuccessfulRules);
        ::Save(stream, PrintRichTree);
        ::Save(stream, PrintCgiParamRelev);
        ::Save(stream, TabbedInput);
        ::Save(stream, VerifyGazetteer);
        ::Save(stream, CompareReqRemote);
        ::Save(stream, NoCache);
        ::Save(stream, SortOutput);

        ::Save(stream, PrintMarkup);

        ::Save(stream, Port);
        ::Save(stream, Host);
        ::Save(stream, CgiInput);

        ::Save(stream, LocalWizardOutFile);
        ::Save(stream, RemoteWizardOutFile);

        ::Save(stream, AppendCgi);

        ::Save(stream, RulesList);
        ::Save(stream, RulesToPrint);

        ::Save(stream, InputFileName);
        ::Save(stream, OutputFileName);

        ::Save(stream, IsTestRun);
        ::Save(stream, DebugMode);
        ::Save(stream, CollectStat);
        ::Save(stream, PrintSrc);

        ::Save(stream, ThreadCount);
    }

    void Load(IInputStream* stream) {
        ::Load(stream, WorkDir);
        ::Load(stream, CustomRuntimeData);
        ::Load(stream, PathToConfig);
        ::Load(stream, EventLogPath);

        ::Load(stream, FastLoader);
        ::Load(stream, PrintDolbilka);
        ::Load(stream, PrintExtraOutput);
        ::Load(stream, PrintLemmaCount);
        ::Load(stream, PrintQtree);
        ::Load(stream, PrintSuccessfulRules);
        ::Load(stream, PrintRichTree);
        ::Load(stream, PrintCgiParamRelev);
        ::Load(stream, TabbedInput);
        ::Load(stream, VerifyGazetteer);
        ::Load(stream, CompareReqRemote);
        ::Load(stream, NoCache);
        ::Load(stream, SortOutput);

        ::Load(stream, PrintMarkup);

        ::Load(stream, Port);
        ::Load(stream, Host);
        ::Load(stream, CgiInput);

        ::Load(stream, LocalWizardOutFile);
        ::Load(stream, RemoteWizardOutFile);

        ::Load(stream, AppendCgi);

        ::Load(stream, RulesList);
        ::Load(stream, RulesToPrint);

        ::Load(stream, InputFileName);
        ::Load(stream, OutputFileName);

        ::Load(stream, IsTestRun);
        ::Load(stream, DebugMode);
        ::Load(stream, CollectStat);
        ::Load(stream, PrintSrc);

        ::Load(stream, ThreadCount);
    }

};
