#include "rule_updater.h"

#include <util/generic/set.h>
#include <util/generic/queue.h>
#include <util/string/join.h>
#include <util/system/mutex.h>
#include <util/system/shellcommand.h>
#include <util/stream/hex.h>
#include <util/folder/path.h>

#include <library/cpp/digest/md5/md5.h>
#include <mapreduce/yt/interface/client.h>
#include <tools/wizard_yt/shard_packer/shard_packer.h>
#include <search/begemot/core/rulefactory.h>

using namespace NYT;

class TStreamHash : public IOutputStream {
private:
    MD5 State;
public:
    void DoWrite(const void* buf, size_t len) override {
        State.Update(buf, len);
        Size += len;
    }
    void DoFinish() override {
        unsigned char digest[16];
        State.Final(&digest[0]);
        TStringOutput ss(Result);
        HexEncode(digest, 16, ss);
    }
    TString Result;
    size_t Size = 0;
};

void SelectAllRulesByRequiredRules(const TVector<TString>& requiredRules, TVector<TString>& result) {
    TQueue<size_t> pendedArtifacts;
    TSet<size_t> selectedArtifacts;
    TSet<TString> selectedRules;
    auto factory = NBg::DefaultRuleFactory();
    for (auto ruleName: requiredRules) {
        auto rule = factory.Find(ruleName);
        Y_VERIFY(rule.Defined(), "rule %s not defined", ruleName.Data());
        selectedArtifacts.emplace(rule.GetRef());
        pendedArtifacts.push(rule.GetRef());
    }
    for (;!pendedArtifacts.empty(); pendedArtifacts.pop()) {
        auto& rule = factory[pendedArtifacts.front()];
        for(const auto& dep: rule.Dependencies) {
            auto depRuleId = dep.first;
            if (selectedArtifacts.find(depRuleId) == selectedArtifacts.end()) {
                selectedArtifacts.emplace(depRuleId);
                pendedArtifacts.push(depRuleId);
            }
        }
    }
    for (auto artifact: selectedArtifacts) {
        selectedRules.emplace(factory[artifact].Root);
    }
    for (auto ruleName: selectedRules) {
        result.push_back(ruleName);
    }
}


TRuleDirUpdater::TRuleDirUpdater(
    TFsPath ruleDir,
    NYT::IClientPtr& client,
    TRuleUpdaterContext& context,
    TVector<TString>& resultPaths
)
    : RuleDir(ruleDir)
    , Client(client)
    , Context(context)
    , ResultPaths(resultPaths) {
}

void TRuleDirUpdater::Process(void*) {
    try {
        PrintDebug("processing directory: " + RuleDir.GetName());
        TStreamHash md5;
        ShardPacker::ProcessDirContents(RuleDir, md5);
        PrintDebug(RuleDir.GetName() + " Size: " + ToString(md5.Size));
        TString ruleName = RuleDir.GetName();
        TString dirPath = Join('/', Context.CachePath, md5.Result);
        TYPath filePath = Join('/', dirPath, ruleName);
        auto lockOptions = NYT::TLockOptions().Waitable(true);
        auto tx = Client->StartTransaction();
        tx->Lock(Context.CachePath, NYT::ELockMode::LM_SHARED, lockOptions)->Wait();
        if (tx->Exists(dirPath)) {
            Client->Set(dirPath + "/@expiration_time", Context.ExpirationTime.ToString());
        }
        tx->Commit();
        if (!Client->Exists(dirPath)) {
            tx = Client->StartTransaction();
            tx->Lock(Context.CachePath, NYT::ELockMode::LM_EXCLUSIVE, lockOptions)->Wait();
            if (!tx->Exists(dirPath)) {
                PrintDebug("Created directory for rule " + RuleDir.GetName());
                TNode attributes = TNode()("expiration_time", Context.ExpirationTime.ToString());
                TCreateOptions options = TCreateOptions().Recursive(true).Attributes(attributes);
                tx->Create(dirPath, NYT::ENodeType::NT_MAP, options);
            }
            tx->Commit();
        }
        if (!Client->Exists(filePath)) {
            tx = Client->StartTransaction();
            tx->Lock(dirPath, NYT::ELockMode::LM_EXCLUSIVE, lockOptions)->Wait();
            if (!tx->Exists(filePath)) {
                PrintDebug("Created file for rule " + RuleDir.GetName());
                tx->Create(filePath, ENodeType::NT_FILE);
                auto writer = tx->CreateFileWriter(filePath);
                ShardPacker::ProcessDirContents(RuleDir, *writer);
            }
            tx->Commit();
        }
        if (!Context.ShardPath.Empty()) {
            TYPath targetPath(Join('/', Context.ShardPath, ruleName));
            Client->Link(filePath, targetPath, TLinkOptions().Recursive(true));
        }
        TGuard<TMutex> g(TRuleDirUpdater::Mtx);
        ResultPaths.push_back(filePath.data());
    }
    catch (const std::exception& e) {
        printf("Exception on %s rule: %s\n", RuleDir.GetName().data(), e.what());
        Context.Failed = true;
        throw e;
    }
}

void TRuleDirUpdater::PrintDebug(const TStringBuf& str) {
    if (Context.EnableDebug) {
        printf("%s\n", str.data());
    }
}

TMutex TRuleDirUpdater::Mtx;

void TSandboxRuleDirUpdater::Process(void* arg) {
    auto cmd = TShellCommand("sky", {"get", Key});
    if (cmd.Run().Wait().GetExitCode() != 0) {
        ythrow yexception() << "Can't receive " << Key << " with sky get: " << cmd.GetError();
    }
    Cerr << RulePath.GetName() << Endl;
    TRuleDirUpdater::Process(arg);
    try {
        RulePath.ForceDelete();
    } catch(...) {
        Cerr << "Can't delete " << RulePath.GetName() << ". Do it manualy" << Endl;
    }
}
