#pragma once

#include "input_tree.h"

#include <util/folder/path.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/string/cast.h>
#include <utility>

namespace NRemorph {

namespace NPrivate {

// Node handler for string printer.
template <typename TSymbol, class TSymbolRepr>
class TNodeToStringAct {
private:
    const TSymbolRepr& SymbolRepr;
    IOutputStream& Output;

public:
    explicit TNodeToStringAct(IOutputStream& output, const TSymbolRepr& symbolRepr)
        : SymbolRepr(symbolRepr)
        , Output(output)
    {
    }

    inline void operator ()(const TInputTree<TSymbol>& node, size_t depth) const {
        Output << "[" << depth << ",";
        SymbolRepr.GetLabel(Output, node.GetSymbol());
        Output << ",l" << node.GetLength()
            << ",b" << node.GetSubBranches()
            << "," << (node.IsAccepted() ? 't' : 'f')
            << "]";
    }
};

// Node handler for dot printer.
template <typename TSymbol, class TSymbolRepr>
class TNodeToDotAct {
private:
    typedef TMap<const TInputTree<TSymbol>*, TString> TNodesMap;

    const TSymbolRepr& SymbolRepr;
    IOutputStream& Output;
    TNodesMap Nodes;

public:
    explicit TNodeToDotAct(IOutputStream& output, const TString& name, const TSymbolRepr& symbolRepr)
        : SymbolRepr(symbolRepr)
        , Output(output)
    {
        Output << "digraph " << name << " {" << Endl
            << "label=\"" << name << "\";" << Endl
            << "rankdir=LR;" << Endl;
    }

    ~TNodeToDotAct() {
        Output << "}" << Endl;
    }

    inline void operator ()(const TInputTree<TSymbol>& node, size_t /*depth*/) {
        const TString& name = GetNode(node);
        for (typename TVector<TInputTree<TSymbol>>::const_iterator child = node.GetNext().begin(); child != node.GetNext().end(); ++child) {
            const TString& childName = NewNode(*child);
            NewEdge(name, childName);
        }
    }

private:
    inline const TString& GetNode(const TInputTree<TSymbol>& node) {
        typename TNodesMap::const_iterator it = Nodes.find(&node);
        if (it == Nodes.end()) {
            return NewNode(node);
        }
        return it->second;
    }

    inline const TString& NewNode(const TInputTree<TSymbol>& node) {
        std::pair<typename TNodesMap::const_iterator, bool> res = Nodes.insert(std::make_pair(&node, ::ToString(Nodes.size())));
        const TString& id = res.first->second;
        Output << "subgraph \"" << id << "\" {" << Endl;
        Output << "rank=same;" << Endl;
        Output << id << " [label=\"";
        SymbolRepr.Label(Output, node.GetSymbol());
        Output << "\"";
        if (node.IsAccepted()) {
            Output << ",style=bold";
        }
        TStringStream attrsPayload;
        SymbolRepr.DotAttrsPayload(attrsPayload, node.GetSymbol(), id);
        if (!attrsPayload.Empty()) {
            Output << "," << attrsPayload.Str();
        }
        Output << "];" << Endl;
        SymbolRepr.DotPayload(Output, node.GetSymbol(), id);
        Output << "}" << Endl;
        return res.first->second;
    }

    inline void NewEdge(const TString& from, const TString& to) {
        Output << "\"" << from << "\" -> \"" << to << "\";" << Endl;
    }
};

} // NPrivate

// Render input to a string representation.
template <class TSymbol, class TSymbolRepr>
inline TString InputTreeToString(const TInputTree<TSymbol>& input, const TSymbolRepr& symbolRepr) {
    TStringStream res;
    NPrivate::TNodeToStringAct<TSymbol, TSymbolRepr> act(res, symbolRepr);
    input.TraverseNodes(act);
    return res.Str();
}

// Render input to a graph in DOT format.
template <class TSymbol, class TSymbolRepr>
inline void InputTreeToDot(const TInputTree<TSymbol>& input, const TString& path, const TSymbolRepr& symbolRepr, const TString& name = "") {
    TOFStream res(path);
    NPrivate::TNodeToDotAct<TSymbol, TSymbolRepr> act(res, name, symbolRepr);
    input.TraverseNodes(act);
}

} // NRemorph
