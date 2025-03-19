#include <library/cpp/charset/recyr.hh>
#include <library/cpp/charset/wide.h>
#include <util/generic/string.h>

#include <kernel/reqerror/reqerror.h>

#include "fixreq.h"
#include "req_node.h"
#include "request.h"

template <typename TChr>
void RemoveOperatorSymbolsImpl(TBasicString<TChr>& s) {
    static const TChr ops[] = {'\"', '\'', '/', '|', '<', '>', '{', '}', '[', ']', '(', ')', '&', '#', '$', '=', '`', '^', 0};

    for (TChr* req = s.begin(); *req; ++req) {
        if (TBasicStringBuf<TChr>(ops).Contains(*req))
            *req = ' ';
    }
}

void RemoveOperatorSymbols(TString& s) {
    RemoveOperatorSymbolsImpl<char>(s);
}

void RemoveOperatorSymbols(TUtf16String& s) {
    RemoveOperatorSymbolsImpl<wchar16>(s);
}

bool FixRequest(ECharset encoding, const TString& request, TString* result, size_t tokenizerVersion) {
    if (!request)
        return false;
    try {
        // Fast track: parse UTF
        if (encoding == CODES_UNKNOWN || encoding == CODES_UTF8) {
            tRequest().Parse(UTF8ToWide(request), nullptr, nullptr, tokenizerVersion);
        } else {
            tRequest().Parse(CharToWide(RecodeToYandex(encoding, request), CODES_YANDEX), nullptr, nullptr, tokenizerVersion);
        }
        return false;
    } catch(const TError& err) {
        if (err.GetCode() == yxREQ_EMPTY)
            return false;
    } catch(const yexception&) {
        return false;
    }

    // Let's fix the request. Operator symbols are the same in all encodings:
    // we can replace them without recoding to and from Unicode.
    *result = request;
    RemoveOperatorSymbols(*result);
    return true;
}
