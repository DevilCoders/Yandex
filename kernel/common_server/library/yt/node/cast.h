#pragma once

#include <library/cpp/yson/node/node.h>

namespace NYT {
    inline bool IsDefined(const TNode& node) {
        return !node.IsUndefined() && !node.IsNull();
    }

    template <class T>
    T ConvertTo(const NYT::TNode& node, T fallback = T()) {
        if (NYT::IsDefined(node)) {
            return node.ConvertTo<T>();
        } else {
            return fallback;
        }
    }

    template <>
    NYT::TNode ConvertTo<NYT::TNode>(const NYT::TNode& node, NYT::TNode fallback);

    template <class T>
    TNode ToNode(const T& object);

    template <class T>
    T FromNode(const TNode& node);
}

namespace NYT {
    template <class T, class P>
    TNode ToNode(const TMaybe<T, P>& object) {
        return object ? ToNode(*object) : TNode::CreateEntity();
    }

    template <class T, class A>
    TNode ToNode(const TVector<T, A>& object) {
        auto result = TNode::CreateList();
        for (auto&& i : object) {
            result.Add(ToNode(i));
        }
        return result;
    }
}
