#include "abstract.h"
#include <kernel/common_server/util/types/interval.h>
#include <library/cpp/json/json_reader.h>

namespace NCS {
    ISettings::ISettings(const ISettingsConfig& /*config*/) {
        RegisterGlobalMessageProcessor(this);
    }

    ISettings::~ISettings() {
        UnregisterGlobalMessageProcessor(this);
    }

    bool ISettings::Process(IMessage* message) {
        TMessageGetSettingValue* messGetValue = dynamic_cast<TMessageGetSettingValue*>(message);
        if (messGetValue) {
            TString value;
            if (GetValueStr(messGetValue->GetSettingName(), value)) {
                messGetValue->SetValue(value);
            }
            return true;
        }
        return false;
    }
}
