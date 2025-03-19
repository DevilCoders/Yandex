#include "helpers.h"

const TRichRequestNode* FindNodeCopy(const TRichRequestNode* srcRoot,const TRichRequestNode* srcNode,
                                     const TRichRequestNode* dstRoot) {
    if (srcRoot == srcNode)
        return dstRoot;

    Y_ASSERT(srcRoot->Children.size() == dstRoot->Children.size());
    for (size_t i = 0; i < srcRoot->Children.size(); ++i) {
        const TRichRequestNode* dstNode = FindNodeCopy(srcRoot->Children[i].Get(), srcNode, dstRoot->Children[i].Get());
        if (dstNode)
            return dstNode;
    }

    return nullptr;
}
