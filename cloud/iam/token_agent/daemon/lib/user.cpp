#include "user.h"

#include <pwd.h>
#include <util/generic/yexception.h>

namespace NTokenAgent {

    template <typename T>
    static TUser ResolveUser(T name_or_id, int (*supplier)(T, passwd*, char*, size_t, passwd**)) {
        passwd passwd = {}, *p = nullptr;
        std::string buf;
        int err;
        do {
            buf.resize(buf.size() + 1000);
            err = supplier(name_or_id, &passwd, buf.data(), buf.length(), &p);
        } while (err == ERANGE);
        if (err) {
            ythrow TSystemError(err);
        }
        if (p) {
            return TUser{p->pw_name, p->pw_uid, p->pw_gid};
        } else {
            // Not existent user
            return TUser{};
        }
    }

    TUser::TUser()
        : Id(std::numeric_limits<ui32>::max())
        , GroupId(std::numeric_limits<ui32>::max())
    {
    }

    TUser::TUser(std::string name, ui32 id, ui32 group_id)
        : Name(std::move(name))
        , Id(id)
        , GroupId(group_id)
    {
    }

    TUser TUser::FromName(const char* name) {
        return ResolveUser(name, getpwnam_r);
    }

    TUser TUser::FromId(ui32 id) {
        return ResolveUser(id, getpwuid_r);
    }

}
