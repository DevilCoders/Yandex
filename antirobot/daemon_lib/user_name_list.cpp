#include "user_name_list.h"

#include <util/stream/input.h>
#include <util/string/strip.h>

namespace NAntiRobot {
    void TUserNameList::Load(IInputStream& in) {
        Set.clear();

        TString line;
        while (in.ReadLine(line)) {
            TStringBuf l = StripString(TStringBuf(line).Before('#'));

            if (l.empty()) {
                continue;
            }

            Set.insert(TString(l.data(), l.size()));
        }

    }

    bool TUserNameList::UserInList(const TString& userName) const {
        return Set.find(userName) != Set.end();
    }
}
