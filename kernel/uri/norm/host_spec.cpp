/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */

#include "host_spec.h"

#include "host_amzn.h"
#include "host_goog.h"
#include "host_twit.h"
#include "host_wiki.h"

namespace Nydx {
namespace NUriNorm {

class SecondLevelDomains
    : public TStrBufSet
{
public:
    SecondLevelDomains()
    {
        Add("co");
        Add("ac");
    }
};

static const SecondLevelDomains secondLevelDomains;
static const NAmzn::THostSpec HostSpecAmzn;
static const NGoog::THostSpec HostSpecGoog;
static const NTwit::THostSpec HostSpecTwit;
static const NWiki::THostSpec HostSpecWiki;

/// returns true if the path has changed
bool THostSpecific::Normalize(TPartNormalizers &norm)
{
    TStringBuf hbuf(norm.Host.Get());
    size_t pos = hbuf.find_last_of('.');
    if (TString::npos == pos)
        return false;

    TStringBuf dom, subdom, hname;

    do {
        dom = hbuf.SplitOffAt(pos).SubStr(1);
        pos = hbuf.find_last_of('.');
        if (TString::npos == pos) {
            hname = hbuf;
            hbuf.Clear();
            break;
        }

        hname = hbuf.SplitOffAt(pos).SubStr(1);
        if (!secondLevelDomains.Has(hname))
            break;

        subdom = hname;
        pos = hbuf.find_last_of('.');
        if (TString::npos == pos) {
            hname = hbuf;
            hbuf.Clear();
            break;
        }

        hname = hbuf.SplitOffAt(pos).SubStr(1);
    }
    while (false);

    // consider host-specific special cases
    if (hname == "amazon")
        return HostSpecAmzn.Normalize(norm);
    if (hname == "google")
        return HostSpecGoog.Normalize(hbuf, norm);
    if (hname == "wikipedia")
        return HostSpecWiki.Normalize(hbuf, norm);
    if (hname == "twitter")
        return HostSpecTwit.Normalize(hbuf, norm);
    // TODO
    // ebay

    return false;
}

}
}
