#pragma once

#include <library/cpp/yson/node/node.h>

#include <util/generic/fwd.h>

namespace NYtHelpers {

    class TScreamingNode {
    public:
        explicit TScreamingNode(const NYT::TNode& node);

        const TScreamingNode operator[](const TStringBuf key) const;

        bool HasKey(const TStringBuf key) const;

        bool IsNull() const;
        bool IsString() const;
        bool IsInt64() const;
        bool IsUint64() const;
        bool IsDouble() const;
        bool IsBool() const;
        bool IsList() const;
        bool IsMap() const;

        const TString& AsString() const;
        i64 AsInt64() const;
        ui64 AsUint64() const;
        double AsDouble() const;
        bool AsBool() const;
        const NYT::TNode::TListType& AsList() const;
        const NYT::TNode::TMapType& AsMap() const;

    private:
        const NYT::TNode& Node;
    };

}
