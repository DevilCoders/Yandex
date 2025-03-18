#pragma once

namespace NBlackbox2::NSessionCodes {
    enum ESessionError {
        OK = 0,
        UNKNOWN,
        INVALID_PARAMS,
        SECOND_INIT,
        BAD_COOKIE_SIZE,
        BAD_COOKIE_BODY,
        INVALID_SOURCE,
        BAD_COOKIE_TS,
        HOST_DONT_MATCH,
        DB_FETCHFAILED,
        DB_EXCEPTION,
        NO_DATA_KEYSPACE,
        KEYRING_FLOOD,
        BAD_KEY_ID,
        KEY_ID_FLOOD,
        KEY_NOT_FOUND,
        KEYSPACE_EMPTY,
        KEYSPACE_FAILED,
        BAD_SIGN,
        BAD_DATA_SIZE,
        BAD_DOMAIN_PREFIX,
        ACCESS_DENIED,
        CANT_CONNECT,
        VERSION_MISMATCH,
        GLOBALLY_LOGGED_OUT
    };
}
