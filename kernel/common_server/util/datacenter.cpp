#include "datacenter.h"

namespace NUtil {

    TString DetectDatacenterCode(const TString& hostName, const TString& defaultValue) {
        if (hostName.find(".man.") != TString::npos) {
            return "MAN";
        } else if (hostName.find(".sas.") != TString::npos) {
            return "SAS";
        } else if (hostName.find(".vla.") != TString::npos) {
            return "VLA";
        } else if (hostName.find("man") != TString::npos) {
            return "MAN";
        } else if (hostName.find("sas") != TString::npos) {
            return "SAS";
        } else if (hostName.find("vla") != TString::npos) {
            return "VLA";
        }
        return defaultValue;
    }

}
