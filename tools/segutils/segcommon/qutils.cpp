#include "qutils.h"

namespace NSegutils {

TOwnerCanonizer CreateAndInitCanonizer(const TString& urlRulesFileName)
{
    TOwnerCanonizer canonizer;
    TFsPath urlrules(urlRulesFileName);

    canonizer.LoadDom2(urlrules / "2ld.list");
    canonizer.UnloadDom2(urlrules / "f2ld.list");

    return canonizer;
}

}

