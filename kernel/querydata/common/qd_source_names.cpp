#include "qd_source_names.h"

#include <util/generic/string.h>

namespace NQueryData {

    // old syntax: main_source_name/namespace (docidban/antispam)
    // new syntax: namespace:main_source_name (antispam:docidban)
    TStringBuf /*mainSrc*/ GetMainSourceName(const TStringBuf& src, TStringBuf& subSrc) {
        if (src.find('/') != TStringBuf::npos) {
            TStringBuf mainSrc;
            src.Split('/', mainSrc, subSrc);
            return mainSrc;
        }

        if (src.find(':') != TStringBuf::npos) {
            subSrc = src;
            TStringBuf mainSrc;
            src.RSplit(':', subSrc, mainSrc);
            return mainSrc;
        }

        return src;
    }

    TStringBuf /*mainSrc*/ GetMainSourceName(const TStringBuf& src) {
        TStringBuf subSrc;
        return GetMainSourceName(src, subSrc);
    }

    TString ComposeSourceName(TStringBuf mainSrc, TStringBuf subSrc) {
        return subSrc ? TString::Join(subSrc, ":", mainSrc) : TString{mainSrc};
    }
}
