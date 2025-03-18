#include "lkey_reader.h"

#include <util/stream/file.h>

namespace NLCookie {
    TStringBuf TFileKeyReader::GetKey(const TStringBuf& keyId) const {
        TKeyMap::const_iterator it = keyMap.find(keyId);
        return it == keyMap.end() ? TStringBuf() : it->second;
    }

    void TFileKeyReader::Load(const TString& path) {
        {
            TFileInput f(path);
            keyStore = f.ReadAll();
        }

        keyMap.clear();
        TStringBuf parse(keyStore);
        while (!parse.empty()) {
            TStringBuf keyId = parse.NextTok(';');
            TStringBuf keyVal = parse.NextTok(';');
            keyMap[keyId] = keyVal;
            parse.NextTok('\n');
        }
    }

}
