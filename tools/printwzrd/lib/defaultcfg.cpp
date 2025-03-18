#include "defaultcfg.h"

TDefaultPrintwzrdCfg::TDefaultPrintwzrdCfg(const TString& workDir, const TString& customRuntimeData, TString rulesList)
    : TWizardConfig(workDir)
{
    SetPureTrieConfig(ResolvePath("wizard/language/pure/pure.lang.config"));
    SetGazetteerDictionary(ResolvePath("wizard/GztBuilder/main.gzt.bin"));
    if (!customRuntimeData.empty()) {
        SetReqWizardRuntimeDir(customRuntimeData);
    }

    if (rulesList.find("Morphology") == TString::npos) {
        rulesList = "Morphology," + rulesList;
    }
    if (rulesList.find("Serialization") == TString::npos && rulesList.find("ExternalMarkup") == TString::npos) {
        if (rulesList.back() != ',') // rulesList is not empty here
            rulesList.push_back(',');
        rulesList += "Serialization";
    }
    SetDefaultRulesList(rulesList);
}
