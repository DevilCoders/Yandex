#pragma once

#include "richnode.h"

// Search for srcNode in srcRoot and return node from dstRoot in the same location.
// dstRoot is a tree copied from srcRoot.
const TRichRequestNode* FindNodeCopy(const TRichRequestNode* srcRoot,const TRichRequestNode* srcNode,
                                     const TRichRequestNode* dstRoot);

