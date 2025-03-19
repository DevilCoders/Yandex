#pragma once

/*
 *  Created on: Dec 7, 2011
 *      Author: albert@
 *
 * $Id$
 */


#include <library/cpp/charset/doccodes.h>
#include <util/generic/strbuf.h>
#include <util/stream/output.h>

namespace Nydx {
namespace NUriNorm {

class TConvUTF8
{
public:
    static bool Normalize(ECharset cset, const TStringBuf &src, IOutputStream &dst);
};

}
}
