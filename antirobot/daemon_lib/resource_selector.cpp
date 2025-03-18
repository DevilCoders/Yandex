#include "resource_selector.h"

#include <library/cpp/uilangdetect/bytld.h>


namespace NAntiRobot {


TMaybe<TResourceKey> TResourceKey::Parse(TStringBuf keyStr) {
    TResourceKey key;

    TStringBuf tld;
    TStringBuf serviceStr;

    if (keyStr.TrySplit('.', tld, serviceStr)) {
        key.Lang = LanguageByTLD(TString(tld));
        key.Service = FromString<EHostType>(serviceStr);

        if (key.Service == EHostType::Count) {
            return Nothing();
        }
    } else {
        key.Lang = LanguageByTLD(TString(keyStr));
    }

    if (key.Lang == LANG_UNK) {
        return Nothing();
    }

    return key;
}


}
