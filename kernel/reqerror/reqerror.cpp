#include "reqerror.h"

#include <util/generic/array_size.h>
#include <util/generic/strbuf.h>

static constexpr TStringBuf ENG_ERRORS[] = {
    {}, //"OK", //-- due to possible checks of emptyness error string
    TStringBuf("Syntax error"),
    TStringBuf("Search request is empty"),
    TStringBuf("Proximity segment is empty"),
    TStringBuf("First number must be less or equal then second ones in search range"),
    TStringBuf("Wrong grammar characteristics"),
    TStringBuf("Not enough memory for the operation"),
    TStringBuf("Disk read error"),
    TStringBuf("Zone is not indexed"),
    TStringBuf("Attribute is not indexed"),
    TStringBuf("Attribute and element are incompatible"),
    TStringBuf("Internal error: zone disbalance"),
    TStringBuf("Previous search result is already deleted"),
    TStringBuf("Disk write error"),
    TStringBuf("Cannot exclude stop-word"),
    TStringBuf("Cannot find these words"),
    TStringBuf("Cannot find requested documents"),
    TStringBuf("Search query results have been removed"),
    TStringBuf("Error in XML request"),
    TStringBuf("Incompatible request parameters"),
    TStringBuf("Operation canceled by timeout"),
    TStringBuf("Unknown error"),
    TStringBuf("Service unavailable"),
    TStringBuf("Requested data not available"),
    TStringBuf("Sending neh request error"),
    TStringBuf("Required db timestamp mismatch"),
    TStringBuf("Query has more than 64 words"),
    TStringBuf("Backends are inaccessible"),
};

static_assert(static_cast<size_t>(yxMAX_ERROR_CODE) == Y_ARRAY_SIZE(ENG_ERRORS), "Size doesn't match");

yxErrorCode TrimErrorCode(yxErrorCode yxCode) {
    return (int(yxCode) < 0 || yxCode >= yxMAX_ERROR_CODE) ? yxUNKNOWN_ERROR : yxCode;
}

TStringBuf yxErrorString(yxErrorCode yxCode) {
   yxCode = TrimErrorCode(yxCode);
   return ENG_ERRORS[yxCode];
}

TStringBuf BAD_DH_PARAM = "Bad dh parameter ";
