#pragma once

#include "config.h"
#include "docsattrs.h"

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/system/defaults.h>

namespace NGroupingAttrs {

class TPrinter {
private:
    TDocsAttrs DocsAttrs;

private:
    void Print(ui32 docid, ui32 attrnum, bool names, bool printname, IOutputStream& out) const;

public:
    TPrinter(const TString& yprefix)
        : DocsAttrs(false, yprefix.data())
    {
    }

    void Print(bool names, IOutputStream& out) const;
    void Print(const char* attrname, bool names, IOutputStream& out) const;
    void Print(const TVector<ui32>& docids, bool names, IOutputStream& out) const;
    void Print(const char* attrname, const TVector<ui32>& docids, bool names, IOutputStream& out) const;

    void Config(IOutputStream& out) const;
    void DocCount(IOutputStream& out) const;

    void Version(IOutputStream& out) const;
    void Format(IOutputStream& out) const;

    void Dump() const;

    static TString Type(TConfig::Type type);
};

}
