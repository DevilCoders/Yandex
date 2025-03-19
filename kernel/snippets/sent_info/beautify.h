#pragma once

#include  <util/generic/string.h>
#include  <util/generic/strbuf.h>

namespace NSnippets
{
    TWtringBuf CutFirstUrls(const TWtringBuf& sent);
    bool LooksLikeUrl(TWtringBuf s);

}
