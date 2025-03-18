#pragma once

#include <util/datetime/base.h>
#include <util/generic/vector.h>

#include <antirobot/daemon_lib/rule_set.h>


const NAntiRobot::TCbbGroupId DEFAULT_CBB_FLAG{183};
const TDuration DEFAULT_CBB_TIMEOUT = TDuration::Seconds(5);
const TString DEFAULT_CBB_HOST = "cbb.yandex.net:80";

struct TSettings {
    TString Cmd;
    TString RulesFile;
    TString RequestLogName;
    bool PrintFullReq = false;
    bool ClearExistRules = false;
    TString CbbHost = DEFAULT_CBB_HOST;
    NAntiRobot::TCbbGroupId CbbFlag = DEFAULT_CBB_FLAG;
    bool JsonOutput = false;
    bool ReqLogTxt = false;
    bool PrintStat = false;
    bool PrintMatched = false;
};

TVector<TString> ReadRules(const TString& fileName);
NAntiRobot::TRuleSet ParseRules(const TVector<TString>& txtRules);

/* For cbb */
bool CheckRules(const TString& rules);
bool ValidateRequest(const TString& request);
bool CheckRequest(const TString& rules, const TString& request);

bool CheckRules(const TSettings& settings);
void CheckSavedReqsWithRules(const TSettings& settings);
void StoreNewRules(const TSettings& settings);

TString PrintOK(bool asJson, const TString& msg);
TString PrintError(bool asJson, const TString& msg);
