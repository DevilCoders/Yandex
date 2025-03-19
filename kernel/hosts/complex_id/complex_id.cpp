#include "complex_id.h"

#include <kernel/urlnorm/urlnorm.h>

#include <util/charset/unidata.h>
#include <util/digest/fnv.h>
#include <util/stream/output.h>
#include <util/stream/format.h>
#include <util/string/cast.h>

ui64 CIFnv(TStringBuf str, ui64 init = FNV64INIT) {
    ui64 res = init;
    for (TStringBuf::const_iterator i = str.begin(), e = str.end(); i != e; ++i)
        res = (res * FNV64PRIME) ^ ((ui8)::ToLower((ui8)*i));
    return res;
}

TComplexId TComplexIdBuilder::Do(TStringBuf host, TStringBuf owner, TStringBuf path, TStringBuf ext) const {
    TStringBuf port;
    if (!path.empty() && path[0] == ':') {
        port = path;
        size_t slash = path.find('/');
        if (slash != TStringBuf::npos) {
            port.Trunc(slash);
            path.Skip(slash);
        } else
            path = TStringBuf();
    }

    ui64 pathId = 0;
    if (!path.empty()) {
        // UrlHashVal expects a slash-starting path
        if (path[0] != '/') {
            UrlHashVal(pathId, TString::Join("/", path).data());
        } else {
            UrlHashVal(pathId, ToString(path).data());
        }
    }

    return Do(host, owner, port, ext, pathId);
}

TComplexId TComplexIdBuilder::Do(TStringBuf host, TStringBuf owner, TStringBuf port, TStringBuf ext, ui64 pathId) const {
    TComplexId id;

    if (!owner.empty())
        id.Owner = CIFnv(owner);

    // Extended owner can be given in two parts
    if (!ext.empty())
        id.Owner = CIFnv(ext, id.Owner);

    id.Path = pathId;

    // Add non-80 m.Port to path hash before prefix
    if (!port.empty() && port != ":80")
        id.Path = CIFnv(port, id.Path ? id.Path : FNV64INIT);

    // Skip 'http://' in any case
    if (host.size() >= 7 && !strnicmp(host.data(), "http://", 7))
        host = host.Skip(7);

    if (!host.empty())
        id.Path = CIFnv(host, id.Path);

    return id;
}

TComplexId ParseComplexId(const TStringBuf& s) {
    size_t c = s.find(':');
    if (c && c != TStringBuf::npos) {
        return TComplexId(IntFromString<ui64, 16>(s.SubStr(0, c)), IntFromString<ui64, 16>(s.SubStr(c + 1)));
    } else
        ythrow yexception() << "Wrong id format, expecting two hex numbers separated by a colon";
}

template <>
void Out<TComplexId>(IOutputStream& out, const TComplexId& id) {
    out << Hex(id.Owner, HF_FULL) << ":" << Hex(id.Path, HF_FULL);
}

ui64 HashOwner(const TString& s) {
    return CIFnv(s);
}
