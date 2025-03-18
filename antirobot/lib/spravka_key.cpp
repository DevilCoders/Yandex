#include "spravka_key.h"

#include <util/string/strip.h>
#include <util/string/hex.h>

namespace NAntiRobot {

TSpravkaKey::TSpravkaKey(IInputStream& key) {
    Key = HexDecode(Strip(key.ReadAll()));
}

TSpravkaKey::TSpravkaKey(const TString& key) {
    Key = HexDecode(Strip(key));
}

} // namespace NAntiRobot
