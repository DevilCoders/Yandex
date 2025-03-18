#pragma once

#include "domtree.h"

#include <util/generic/fwd.h>
#include <util/generic/string.h>

class IInputStream;

namespace NDomTree {
    const size_t DEFAULT_MAX_ALLOWED_TREE_DEPTH = 500;

    TDomTreePtr BuildTree(IInputStream& htmlStream, const TString& url, const TString& charset = "utf-8", size_t maxAllowedTreeDepth = DEFAULT_MAX_ALLOWED_TREE_DEPTH);
    TDomTreePtr BuildTree(const TStringBuf& html, const TString& url, const TString& charset = "utf-8", size_t maxAllowedTreeDepth = DEFAULT_MAX_ALLOWED_TREE_DEPTH);

}
