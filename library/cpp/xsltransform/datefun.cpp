#include <contrib/libs/libxslt/libxslt/extensions.h>
#include <libxml/xpathInternals.h>
#include <util/generic/string.h>
#include <util/datetime/base.h>
#include <util/string/cast.h>
#include "datefun.h"

bool ParseAnyDateTime(const char* date, size_t dateLen, time_t& utcTime) {
    // bool ParseISO8601DateTimeDeprecated(const char* date, size_t dateLen, time_t& utcTime);
    typedef bool (*TParseDateFun)(const char* date, size_t dateLen, time_t& result);
    const TParseDateFun parseDateFunctions[] = {
        ParseHTTPDateTimeDeprecated,
        ParseRFC822DateTimeDeprecated,
        ParseISO8601DateTimeDeprecated,
        ParseX509ValidityDateTimeDeprecated};

    bool ok = false;
    for (auto fun : parseDateFunctions) {
        if (fun(date, dateLen, utcTime)) {
            ok = true;
            break;
        }
    }
    return ok;
}

bool ParseAnyDateTime(const char* date, time_t& utcTime) {
    return ParseAnyDateTime(date, strlen(date), utcTime);
}

void xsltUniformDate(xmlXPathParserContextPtr ctxt, int nargs) {
    CHECK_ARITY(1);
    xmlChar* arg = xmlXPathPopString(ctxt);

    time_t time = 0;
    bool ok = ParseAnyDateTime((const char*)arg, time);
    Y_UNUSED(ok); // assume pubdate = 0 for date we cannot parse
    xmlFree(arg);

    // xmlXPathReturnNumber returns double so we convert to string
    TString timeString = ToString(time);
    valuePush(ctxt, xmlXPathNewString((xmlChar*)timeString.data()));
}

// actual return type is
// typedef void (*TxmlXPathFunction) (xmlXPathParserContextPtr ctxt, int nargs);
void* GetUniformDateFun() {
    return (void*)xsltUniformDate;
}
