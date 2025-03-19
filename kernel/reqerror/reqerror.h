#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>
#include <util/generic/yexception.h>

/*
 * This enum is duplicated in the Perl code of Report, so that messages
 * localization can be easily implemented. If you change this enum,
 * please, adjust arcadia/web/report/lib/YxWeb/Search/Errors.pm accordingly
 * or notify the SERP group
 */
enum yxErrorCode {
    yxOK = 0,
    yxSYNTAX_ERROR,            // 1
    yxREQ_EMPTY,               // 2
    yxEMPTY_PROXIMITY,         // 3
    yxWRONG_PROXIMITY,         // 4
    yxWRONG_GRAM_CHAR,         // 5
    yxNO_MEM,                  // 6
    yxBAD_LOAD,                // 7
    yxELEM_NOT_INDEXED,        // 8
    yxATTR_NOT_INDEXED,        // 9
    yxATTR_ELEM_INCOMPATIBLE,  // 10
    yxBAD_ZONE,                // 11
    yxBAD_PREV_REQUEST,        // 12
    yxBAD_SAVE,                // 13
    yxSTOP_WORD,               // 14
    yxNULL_RESULT,             // 15
    yxNOTFOUND_DOCS,           // 16
    yxSAVED_REQ_REMOVED,       // 17
    yxXML_REQ_ERROR,           // 18
    yxINCOMPATIBLE_REQ_PARAMS, // 19
    yxTIMEOUT,                 // 20
    yxUNKNOWN_ERROR,           // 21
    yxSERVICE_UNAVAILABLE,     // 22
    yxDATA_NOT_AVAILABLE,      // 23
    yxNEH_REQ_ERROR,           // 24
    yxWRONG_TIMESTAMP,         // 25
    yxLONG_QUERY,              // 26
    yxBACKENDS_INACCESSIBLE,   // 27
    // together with appending new error code here
    // MUST BE appended error code description into engErrors[] at reqerror.cpp
    yxMAX_ERROR_CODE
};

Y_DECLARE_PODTYPE(yxErrorCode);

TStringBuf yxErrorString(yxErrorCode yxCode);
yxErrorCode TrimErrorCode(yxErrorCode yxCode);

// return true, if exist possibility for success response from shard replica
constexpr bool IsLocalYxError(int yxCode) noexcept {
    return (
        yxCode == yxSERVICE_UNAVAILABLE ||
        yxCode == yxNEH_REQ_ERROR ||
        yxCode == yxWRONG_TIMESTAMP ||
        yxCode == yxNO_MEM ||
        yxCode == yxBAD_LOAD ||
        yxCode == yxBACKENDS_INACCESSIBLE
    );
}

constexpr bool IsFastYxError(int yxCode) noexcept {
    return (
        yxCode == yxNEH_REQ_ERROR ||
        yxCode == yxWRONG_TIMESTAMP ||
        yxCode == yxNO_MEM ||
        yxCode == yxBAD_LOAD ||
        yxCode == yxBACKENDS_INACCESSIBLE
    );
}

// copyable version of TError
struct TExtendedErrorInfo {
    TString Message;
    yxErrorCode Code = yxOK;
};


class TError: public yexception {
public:
    explicit TError(yxErrorCode code)
        : Code(code)
    {
        Append(yxErrorString(code));
        Append(TStringBuf(". "));
    }

    yxErrorCode GetCode() const noexcept {
        return Code;
    }

private:
    const yxErrorCode Code;
};

class TSyntaxError: public TError {
public:
    inline TSyntaxError()
        : TError(yxSYNTAX_ERROR)
    {
    }
};

extern TStringBuf BAD_DH_PARAM;
