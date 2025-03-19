#include "cast.h"

#include <util/datetime/base.h>

template <>
NYT::TNode NYT::ConvertTo<NYT::TNode>(const NYT::TNode& node, NYT::TNode fallback) {
    if (NYT::IsDefined(node)) {
        return node;
    } else {
        return fallback;
    }
}

#define DEFINE_NODE_CAST(T)                 \
template <>                                 \
NYT::TNode NYT::ToNode(const T& object) {   \
    return object;                          \
}

DEFINE_NODE_CAST(double)

#undef DEFINE_NODE_CAST

template <>
NYT::TNode NYT::ToNode(const TInstant& object) {
    return object.MicroSeconds();
}
