#include <library/cpp/string_utils/tskv_format/escape.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size) {
    const auto src = TStringBuf(data, size);
    TString buffer, res;

    NTskvFormat::Unescape(NTskvFormat::Escape(src, buffer), res);
    Y_ENSURE(src == res);
    return 0;
}
