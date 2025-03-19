#include "screaming_node.h"

#include <util/stream/output.h>

namespace NYtHelpers {

    TScreamingNode::TScreamingNode(const NYT::TNode& node)
    : Node(node)
    {
    }

    const TScreamingNode TScreamingNode::operator[](const TStringBuf key) const {
        try {
            return TScreamingNode(Node[key]);
        } catch (NYT::TNode::TTypeError& exc) {
            Cerr << "Exception trying to access sub-node \"" << key << "\": " << exc.what() << '\n';
            throw exc;
        }
    }

    bool TScreamingNode::HasKey(const TStringBuf key) const {
        return Node.HasKey(key);
    }

    bool TScreamingNode::IsNull() const {
        return Node.IsNull();
    }

    bool TScreamingNode::IsString() const {
        return Node.IsString();
    }
    bool TScreamingNode::IsInt64() const {
        return Node.IsInt64();
    }
    bool TScreamingNode::IsUint64() const {
        return Node.IsUint64();
    }
    bool TScreamingNode::IsDouble() const {
        return Node.IsDouble();
    }
    bool TScreamingNode::IsBool() const {
        return Node.IsBool();
    }
    bool TScreamingNode::IsList() const {
        return Node.IsList();
    }
    bool TScreamingNode::IsMap() const {
        return Node.IsMap();
    }

    const TString& TScreamingNode::AsString() const {
        return Node.AsString();
    }
    i64 TScreamingNode::AsInt64() const {
        return Node.AsInt64();
    }
    ui64 TScreamingNode::AsUint64() const {
        return Node.AsUint64();
    }
    double TScreamingNode::AsDouble() const {
        return Node.AsDouble();
    }
    bool TScreamingNode::AsBool() const {
        return Node.AsBool();
    }
    const NYT::TNode::TListType& TScreamingNode::AsList() const {
        return Node.AsList();
    }
    const NYT::TNode::TMapType& TScreamingNode::AsMap() const {
        return Node.AsMap();
    }

}
