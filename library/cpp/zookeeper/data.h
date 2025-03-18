#pragma once

#include <contrib/libs/zookeeper/generated/zookeeper.jute.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/string/printf.h>

namespace NZooKeeper {
    // wrap zookeeper C data structures
    struct TId : Id {
        TId() {
            Zero(*this);
        }

        TId(const Id& other) {
            memcpy(this, &other, sizeof(*this));
        }

        TId(const char* theScheme, const char* theId) {
            Zero(*this);
            Id::scheme = const_cast<char*>(theScheme);
            Id::id = const_cast<char*>(theId);
        }

        TString ToString() const {
            return Sprintf("Id scheme:%s id:%s", scheme, id);
        }
    };

    struct TACL : ACL {
        TACL() {
            Zero(*this);
        }

        TACL(const ACL& other) {
            memcpy(this, &other, sizeof(*this));
        }

        TACL(int thePerms, TId theId) {
            Zero(*this);
            ACL::perms = thePerms;
            ACL::id = theId;
        }

        TString ToString() const {
            return Sprintf("ACL perms:%d id:%s", perms, TId(id).ToString().data());
        }
    };

    struct TStat : Stat {
        TStat() {
            Zero(*this);
        }

        TStat(const Stat& other) {
            memcpy(this, &other, sizeof(*this));
        }

        TStat(i64 czxid1, i64 mzxid1, i64 ctime1, i64 mtime1, i32 version1, i32 cversion1,
              i32 aversion1, i64 ephemeralOwner1, i32 dataLength1, i32 numChildren1, i64 pzxid1) {
            Zero(*this);
            Stat::czxid = czxid1;
            Stat::mzxid = mzxid1;
            Stat::ctime = ctime1;
            Stat::mtime = mtime1;
            Stat::version = version1;
            Stat::cversion = cversion1;
            Stat::aversion = aversion1;
            Stat::ephemeralOwner = ephemeralOwner1;
            Stat::dataLength = dataLength1;
            Stat::numChildren = numChildren1;
            Stat::pzxid = pzxid1;
        }

        TString ToString() const;
    };

    using TStatPtr = TAutoPtr<TStat>;
}
