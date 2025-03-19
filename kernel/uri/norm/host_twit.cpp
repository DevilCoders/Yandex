/*
 *  Created on: Jan 28, 2012
 *      Author: albert@
 *
 * $Id$
 */

#include "coll.h"
#include "host_twit.h"

namespace Nydx {
namespace NUriNorm {
namespace NTwit {

/**
 * Normalizes the Twitter URL.
 */
class THostSpecImpl
{
public:
    static bool NormalizeHome(TPartNormalizers &norm);
};

bool THostSpec::Normalize(const TStringBuf &hpref, TPartNormalizers &norm) const
{
    if (hpref == "" || hpref == "www")
        return THostSpecImpl::NormalizeHome(norm);

    norm.Frag.Keep();
    return true;
}

bool THostSpecImpl::NormalizeHome(TPartNormalizers &norm)
{
    TStringBuf frag = norm.Frag.GetSet();
    if (frag.empty())
        return false;

    bool changed = false;
    if ('!' == frag[0]) {
        frag.Skip(1);
        changed = true;
    }

    TStringBuf frag1, frag2;
    frag.Split('?', frag1, frag2);

    TString newpath;
    if (!frag1.empty() && !norm.Path.Disabled()) {
        TStringBuf path = norm.Path.GetBuf();
        bool ptail = '/' == path.back();
        const bool fhead = '/' == frag1[0];
        if (ptail && fhead) {
            path.Chop(1);
            ptail = false;
        }
        if (!ptail && !fhead)
            newpath = TString::Join(path, "/", frag1);
        else if (path.empty())
            newpath = frag1;
        else
            newpath = TString::Join(path, frag1);
        norm.Path.Set(newpath);
        frag1.Clear();
    }

    if (!frag2.empty() && !norm.Query.Disabled()) {
        norm.Query.Set(frag2);
        frag2.Clear();
    }

    if (!frag1 && !frag2)
        norm.Frag.Reset();
    else if (!frag2)
        norm.Frag.SetMem(frag1);
    else if (!frag1)
        norm.Frag.SetMem(frag.Last(frag2.length() + 1));
    else if (changed)
        norm.Frag.SetMem(frag);

    return true;
}

}
}
}
