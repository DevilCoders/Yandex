/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */

#include "host.h"

namespace Nydx {
namespace NUriNorm {

bool THostNormalizerBase::NormalizeHost(TStringBuf &host, const TFlags &flags) const
{
    const bool removeWWW = flags.GetRemoveWWW();
    static const TStringBuf www(TStringBuf("www."));

    TStringBuf res = UnmirrorHost(host);
    if (res.data() == nullptr) {
        if (!removeWWW || !host.StartsWith(www))
            return false;
        host.Skip(4);
        res = UnmirrorHost(host);
        if (res.data() == nullptr)
            return true;
    }

    if (removeWWW && res.StartsWith(www))
        res.Skip(4);

    host = res;
    return true;
}


bool THostNormalizerBase::SetMirrorFile(const char *mirfile)
{
    if (nullptr != MirMap_.Get() || nullptr != MirTrie_.Get())
        return false;
    const TStringBuf file(mirfile);
    if (file.EndsWith(TStringBuf(".hash")))
        MirMap_.Reset(new mirrors_mapped(mirfile));
    else if (file.EndsWith(TStringBuf(".trie")))
        MirTrie_.Reset(new TMirrorsMappedTrie(mirfile));
    else
        throw yexception()
            << "Mirror file name must end in .trie or .hash: " << file;
    return true;
}

}
}
