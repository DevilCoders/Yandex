#include "printer.h"

#include "categseries.h"
#include "metainfo.h"
#include "metainfos.h"

#include <util/stream/file.h>
#include <util/string/printf.h>

namespace NGroupingAttrs {

TString TPrinter::Type(TConfig::Type type) {
    switch (type) {
        case TConfig::I16:
            return "i16";
        case TConfig::I32:
            return "i32";
        default:
            return "i64";
    }
}

void TPrinter::Print(ui32 docid, ui32 attrnum, bool names, bool printname, IOutputStream& out) const {
    TCategSeries categs;
    const TMetainfo& meta = *DocsAttrs.Metainfos().Metainfo(attrnum);
    DocsAttrs.DocCategs(docid, attrnum, categs);
    const char* attrname = DocsAttrs.Config().AttrName(attrnum);
    for (const TCateg* c = categs.Begin(); c != categs.End(); ++c) {
        out << docid;

        if (printname) {
            out << '\t' << attrname;
        }

        out << '\t' << (i64)*c;

        if (names) {
            out << '\t' << meta.Categ2Name(*c);
        }

        out << Endl;
    }
}

void TPrinter::Print(bool names, IOutputStream& out) const {
    for (ui32 docid = 0; docid < DocsAttrs.DocCount(); ++docid) {
        for (ui32 attrnum = 0; attrnum < DocsAttrs.Config().AttrCount(); ++attrnum) {
            Print(docid, attrnum, names, true, out);
        }
    }
}

void TPrinter::Print(const char* attrname, bool names, IOutputStream& out) const {
    if (!DocsAttrs.Config().HasAttr(attrname)) {
        ythrow yexception() << "Attribute " << attrname << " doesn't exist";
    }

    ui32 attrnum = DocsAttrs.Config().AttrNum(attrname);
    for (ui32 docid = 0; docid < DocsAttrs.DocCount(); ++docid) {
        Print(docid, attrnum, names, true, out);
    }
}

void TPrinter::Print(const TVector<ui32>& docids, bool names, IOutputStream& out) const {
    for (TVector<ui32>::const_iterator docid = docids.begin(); docid != docids.end(); ++docid) {
        for (ui32 attrnum = 0; attrnum < DocsAttrs.Config().AttrCount(); ++attrnum) {
            Print(*docid, attrnum, names, true, out);
        }
    }
}

void TPrinter::Print(const char* attrname, const TVector<ui32>& docids, bool names, IOutputStream& out) const {
    if (!DocsAttrs.Config().HasAttr(attrname)) {
        ythrow yexception() << "Attribute " << attrname << " doesn't exist";
    }

    ui32 attrnum = DocsAttrs.Config().AttrNum(attrname);
    for (TVector<ui32>::const_iterator docid = docids.begin(); docid != docids.end(); ++docid) {
        Print(*docid, attrnum, names, true, out);
    }
}

void TPrinter::Config(IOutputStream& out) const {
    const TConfig& config = DocsAttrs.Config();
    for (ui32 attrnum = 0; attrnum < config.AttrCount(); ++attrnum) {
        out << config.AttrName(attrnum) << '\t' << Type(config.AttrType(attrnum)) << Endl;
    }
}

void TPrinter::DocCount(IOutputStream& out) const {
    out << DocsAttrs.DocCount() << Endl;
}

void TPrinter::Version(IOutputStream& out) const {
    out << DocsAttrs.Version() << Endl;
}

void TPrinter::Format(IOutputStream& out) const {
    switch (DocsAttrs.Format()) {
        case 0:
            out << "index" << Endl;
            break;
        case 1:
            out << "search" << Endl;
            break;
    }
}

void TPrinter::Dump() const {
    const TConfig& config = DocsAttrs.Config();
    for (ui32 attrnum = 0; attrnum < config.AttrCount(); ++attrnum) {
        TFixedBufferFileOutput out(Sprintf("%s.d2c",config.AttrName(attrnum)).data());
        for (ui32 docid = 0; docid < DocsAttrs.DocCount(); ++docid) {
            Print(docid, attrnum, false, false, out);
        }
    }
}

}
