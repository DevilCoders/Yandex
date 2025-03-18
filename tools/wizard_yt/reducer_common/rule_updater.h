#pragma once
#include <util/generic/string.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>
#include <util/folder/path.h>
#include <util/datetime/base.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <mapreduce/yt/interface/common.h>

struct TRuleUpdaterContext {
    TString ShardPath;
    TString CachePath;
    TInstant ExpirationTime;
    bool EnableDebug = false;
    bool Failed = false;
};

void SelectAllRulesByRequiredRules(const TVector<TString>& requiredRules, TVector<TString>& result);

class TRuleDirUpdater : public IObjectInQueue {
private:
    static TMutex Mtx;
    TFsPath RuleDir;
    NYT::IClientPtr& Client;
    TRuleUpdaterContext& Context;
    TVector<TString>& ResultPaths; // local file with cypress paths
public:
    TRuleDirUpdater(
        TFsPath ruleDir,
        NYT::IClientPtr& client,
        TRuleUpdaterContext& context,
        TVector<TString>& resultPaths
    );
    virtual void Process(void*);
private:
    void PrintDebug(const TStringBuf& str);
};

class TSandboxRuleDirUpdater : public TRuleDirUpdater {
private:
    TFsPath RulePath;
    TString Key;
public:
    TSandboxRuleDirUpdater(
        const TFsPath& path,
        const TString& key,
        NYT::IClientPtr& client,
        TRuleUpdaterContext& context,
        TVector<TString>& resultPaths)
            : TRuleDirUpdater(
                path,
                client,
                context,
                resultPaths)
            , RulePath(path)
            , Key(key)
    {}

    virtual void Process(void* arg);
};
