#include "group.h"

#include <grp.h>
#include <util/generic/yexception.h>

namespace NTokenAgent {

    template <typename T>
    static TGroup ResolveGroup(T name_or_id, int (*supplier)(T, group*, char*, size_t, group**)) {
        group gr = {}, *p = nullptr;
        std::string buf;
        int err;
        do {
            buf.resize(buf.size() + 1000);
            err = supplier(name_or_id, &gr, buf.data(), buf.length(), &p);
        } while (err == ERANGE);
        if (err) {
            ythrow TSystemError(err);
        }
        if (p) {
            return TGroup{p->gr_name, p->gr_gid};
        } else {
            // Not existent group
            return TGroup{};
        }
    }

    TGroup::TGroup()
        : Id(std::numeric_limits<ui32>::max())
    {
    }

    TGroup::TGroup(std::string name, ui32 id)
        : Name(std::move(name))
        , Id(id)
    {
    }

    TGroup TGroup::FromName(const char* name) {
        return ResolveGroup(name, getgrnam_r);
    }

    TGroup TGroup::FromId(ui32 id) {
        return ResolveGroup(id, getgrgid_r);
    }
}
