#include "tree.h"

namespace NHtmlTree {
    TElementNode::TElementNode(TElementNode* p, const THtmlChunk* e, TTree* t)
        : TNode(NODE_ELEMENT, p, e)
        , Children(*t->GetAllocator())
    {
    }

}
