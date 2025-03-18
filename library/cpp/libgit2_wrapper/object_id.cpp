#include "object_id.h"

#include <util/stream/output.h>
#include <util/generic/string.h>

namespace NLibgit2 {
    TString TOid::ToString() const {
        TString buf;
        buf.ReserveAndResize(GIT_OID_HEXSZ + 1);
        git_oid_tostr(buf.begin(), buf.Size(), &Oid_);
        buf.pop_back();
        return buf;
    }
} // namespace NLibgit2

template <>
void Out<NLibgit2::TOid>(IOutputStream& out, const NLibgit2::TOid& oid) {
    out << oid.ToString();
}

template <>
struct THash<NLibgit2::TOid> {
    size_t operator()(const NLibgit2::TOid& oid) const {
        return oid.Hash();
    }
};
