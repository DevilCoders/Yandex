#include "qd_util.h"

#include <google/protobuf/text_format.h>

#include <util/datetime/base.h>
#include <util/folder/dirut.h>
#include <util/folder/path.h>

#include <util/system/fasttime.h>
#include <util/system/mem_info.h>

namespace NQueryData {

ui64 CurrentMemoryUsage() {
    return NMemInfo::GetMemInfo().RSS;
}

time_t FastNow() {
    return InterpolatedMicroSeconds() / 1000000;
}

TString GetPath(const TString& defdir, const TString& file) {
    if (file.StartsWith('/'))
        return file;
    if (!defdir)
        return TFsPath(NFs::CurrentWorkingDirectory()) / file;
    return TFsPath(defdir) / file;
}

TString TimestampToString(time_t tstamp) {
    struct tm t;
    GmTimeR(&tstamp, &t);
    return Strftime("%Y.%m.%d %T", &t).append(TStringBuf(" UTC")); // musl doesn't output timezone
}

TString HumanReadableProtobuf(const google::protobuf::Message& d, bool compact) {
    google::protobuf::TextFormat::Printer printer;
    printer.SetUseUtf8StringEscaping(true);
    printer.SetUseShortRepeatedPrimitives(true);
    printer.SetSingleLineMode(compact);

    TString s;
    printer.PrintToString(d, &s);
    return s;
}

}
