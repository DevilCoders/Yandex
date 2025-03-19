#include "catclosure.h"

#include <util/stream/file.h>

TCatClosure::TCatClosure(const char* fname)
    : Zero(0)
{
    int lineno = 0;
    TFileInput c2p(fname);
    TString buf;

    while (c2p.ReadLine(buf)) {
        ui32 cat, par, n;
        lineno++;
        if (2 != sscanf(buf.data(), "%u %u%n", &cat, &par, &n) || buf.size() != n) {
            ythrow yexception() << "error in cattransit: line " <<  lineno;
        }
        if (cat <= 0 || par <= 0)
            continue;
        Map[cat].push_back(par);
    }

    TCatTransit::iterator b, e = Map.end(), p;
    for (b = Map.begin(); b != e; ++b) {
        TCatSet Parents;
        ui32 par = b->second[0];
        Parents.insert((ui32)b->first);
        Parents.insert(par);
        TCatSet::iterator pe = Parents.end();
        while (((p = Map.find(par)) != e) && (Parents.find(p->second[0]) == pe)) {
            par = p->second[0];
            Parents.insert(par);
            b->second.push_back(par);
        }
        b->second.push_back(0);
    }
}

const ui32* TCatClosure::Get(ui32 cat) const {
    TCatTransit::const_iterator v = Map.find(cat);
    if (v == Map.end())
        return &Zero;
    return &(*(v->second.begin()));
}

void TCatClosure::DoClosure(const TCatSet& in, TCatSet* out) const {
    TCatSet::const_iterator b, e = in.end();
    for (b = in.begin(); b != e; ++b) {
        out->insert(*b);
        const ui32 *cats = Get(*b);
        while (*cats)
            out->insert(*cats++);
    }
}
