#include "target_type_parameters.h"

#include <util/digest/fnv.h>
#include <util/stream/str.h>

ui64 Checksum(TTargetTypeParameters::TIterator param) {
    TStringStream ss;
    while (param.Next()) {
        ss << "<";
        TTargetTypeParameters::TPath path = param.CurrentPath();
        for (size_t i = 1; i < path.size(); ++i) {
            ss << "<" << path.at(i);
        }
    }
    //Cerr << ss.Str() << "" << FnvHash<ui64>(~ss, +ss) <<  Endl;
    return FnvHash<ui64>(ss.Data(), ss.Size());
}
