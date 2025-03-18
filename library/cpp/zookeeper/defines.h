#pragma once

#include <util/generic/vector.h>

#include "data.h"

namespace NZooKeeper {
    enum ECreateMode {
        CM_PERSISTENT = 0,
        CM_PERSISTENT_SEQUENTIAL = 2,
        CM_EPHEMERAL = 1,
        CM_EPHEMERAL_SEQUENTIAL = 3
    };

    enum EEventType {
        ET_NONE = -1,
        ET_NODE_CREATED = 1,
        ET_NODE_DELETED = 2,
        ET_NODE_DATA_CHANGED = 3,
        ET_NODE_CHILDREN_CHANGED = 4,
        ET_NOT_WATCHING = -2
    };

    enum EKeeperState {
        KS_DISCONNECTED = 0,
        KS_NO_SYNC_CONNECTED = 1,
        KS_SYNC_CONNECTED = 3,
        KS_AUTH_FAILED = 4,
        KS_CONNECTED_READ_ONLY = 5,
        KS_SASL_AUTHENTICATED = 6,
        KS_EXPIRED = -112
    };

    enum ELogLevel {
        LL_NONE,
        LL_ERROR,
        LL_WARN,
        LL_INFO,
        LL_DEBUG
    };

    const int PERM_READ = 1 << 0;
    const int PERM_WRITE = 1 << 1;
    const int PERM_CREATE = 1 << 2;
    const int PERM_DELETE = 1 << 3;
    const int PERM_ADMIN = 1 << 4;
    const int PERM_ALL = 0x1f;

    const TId ID_ANYONE = TId("world", "anyone");
    const TId ID_AUTH = TId("auth", "");

    const TACL ACL_ALL = TACL(PERM_ALL, ID_ANYONE);
    const TVector<TACL> ACLS_ALL = TVector<TACL>(1, ACL_ALL);

}

// GENERATE_ENUM_SERIALIZATION
const TString& ToString(NZooKeeper::ECreateMode);
const TString& ToString(NZooKeeper::EEventType);
const TString& ToString(NZooKeeper::EKeeperState);

bool FromString(const TString&, NZooKeeper::ECreateMode&);
bool FromString(const TString&, NZooKeeper::EEventType&);
bool FromString(const TString&, NZooKeeper::EKeeperState&);
