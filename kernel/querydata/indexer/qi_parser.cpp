#include "qi_parser.h"

namespace NQueryData {

    bool DoCheckSize(TString& s, size_t data, size_t limit, TStringBuf name) {
        if (!limit || data <= limit)
            return true;

        TStringStream sout;
        sout << name << " too big (" << data << " > " << limit << "), use -f option to change limits";
        s = sout.Str();

        return false;
    }

}
