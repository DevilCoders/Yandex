#include "drawrichtree.h"

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/nodeiterator.h>
#include <library/cpp/charset/wide.h>

#include <util/generic/string.h>
#include <util/stream/input.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/subst.h>

namespace {

typedef TAllNodesIterator TGraphVizIterator;

TUtf16String NodeDeclaration(const TRichRequestNode& node, const TUtf16String& label)
{
    TUtf16String appearance;
    if (IsAttribute(node)) {
        appearance = u", shape=\"box\"";
    } else if (IsWord(node)) {
        appearance = u", shape=\"plaintext\"";
        if (node.IsStopWord()) {
            appearance += u" style=\"filled\" fillcolor=\"gray80\"";
        }
    }  else  {
        appearance = u", shape=\"diamond\"";
    }
    return TUtf16String::Join(ToWtring<TRichNodeId>(node.GetId()), u" [label = \"", label, u"\"", appearance, u"]");
}

TUtf16String GetOperatorText(TCompareOper oper) {
    switch (oper) {
        case cmpLT:
            return u"<";
        case cmpLE:
            return u"<=";
        case cmpEQ:
            return u":";
        case cmpGT:
            return u">";
        case cmpGE:
            return u">=";
        default:
            Y_UNREACHABLE();
            return TUtf16String();
    }
}

TUtf16String GetNodeText(const TRichRequestNode& node, bool firstPass)
{
    if (!firstPass) {
        return ToWtring<TRichNodeId>(node.GetId());
    }
    if (IsAttribute(node)) {
        const TUtf16String& text(node.GetText());
        const TUtf16String attrValue = (text.size() < 2 || !text.StartsWith(u"\"")) ? text : TUtf16String(text, 1, text.size() - 1);
        return NodeDeclaration(node, TUtf16String::Join(node.GetTextName(), GetOperatorText(node.OpInfo.CmpOper), attrValue));
    } else if (IsWord(node)) {
        return NodeDeclaration(node, node.GetText());
    } else if (node.Op() == oAnd) {
        return NodeDeclaration(node, u"AND");
    } else if (node.Op() == oWeakOr || node.Op() == oOr) {
        return NodeDeclaration(node, u"OR");
    } else if (node.Op() == oRestrDoc) {
        return NodeDeclaration(node, u"<<");
    } else if (node.Op() == oRefine) {
        return NodeDeclaration(node, u"<-");
    } else if (node.Op() == oAndNot) {
        return NodeDeclaration(node, u"~~");
    } else if (node.Op() == oZone) {
        return NodeDeclaration(node, TUtf16String::Join(u"ZONE:", node.GetTextName()));
    } else {
        return NodeDeclaration(node, node.GetText());
    }
    return TUtf16String();
}

void PrintLinkText(const TRichRequestNode& start, const TRichRequestNode& end, const TString& text, IOutputStream& out, ECharset encoding)
{
    out << WideToChar(GetNodeText(start, false), encoding) << "->" << WideToChar(GetNodeText(end, false), encoding) << text << ";" << Endl;
}

void DeclareNodes(TRichTreePtr& tree, IOutputStream& out, ECharset encoding)
{
    for (TGraphVizIterator it(tree->Root); !it.IsDone(); ++it) {
        const TRichRequestNode& node = *it;
        out << WideToChar(GetNodeText(node, true), encoding) << ";" << Endl;
    }
}

void PrintNodeLinks(const TRichNodePtr& node, IOutputStream& out, ECharset encoding)
{
    for (size_t i = 0; i < node->Children.size(); ++i) {
        PrintLinkText(*node, *node->Children[i], "", out, encoding);
        PrintNodeLinks(node->Children[i], out, encoding);
    }
    // Synonyms
    for (NSearchQuery::TForwardMarkupIterator<TSynonym, true> it(node->Markup()); !it.AtEnd(); ++it) {
        if (IsAndOp(*node) && !node->Children.empty()) {
            PrintLinkText(*node->Children[it->Range.Beg], *it.GetData().SubTree, "[label=\"Beg\", style=\"dashed\", fontsize=8]", out, encoding);
            PrintLinkText(*node->Children[it->Range.End], *it.GetData().SubTree, "[label=\"End\", style=\"dashed\", fontsize=8]", out, encoding);
        } else {
            PrintLinkText(*node, *it.GetData().SubTree, "[style=\"dashed\"]", out, encoding);
        }
        PrintNodeLinks(it.GetData().SubTree, out, encoding);
    }
    // MiscOps
    for (size_t i = 0; i < node->MiscOps.size(); ++i) {
        PrintLinkText(*node, *node->MiscOps[i], "[style=\"dotted\", color=\"red\"]", out, encoding);
        out << "{rank = same; " << WideToChar(GetNodeText(*node, false), encoding) << "; " << WideToChar(GetNodeText(*node->MiscOps[i], false), encoding) << "; }" << Endl;
        PrintNodeLinks(node->MiscOps[i], out, encoding);
    }
}

void DeclareLinks(TRichTreePtr& tree, IOutputStream& out, ECharset encoding)
{
    PrintNodeLinks(tree->Root, out, encoding);
}

} // namespace

namespace NDrawRichTree {

void GenerateGraphViz (const TString& qtree, IOutputStream& out, ECharset encoding)
{
    TString line(qtree);
    SubstGlobal(line, "+", "%2B");
    CGIUnescape(line);
    TString unescaped(line);
    TBinaryRichTree packedTree = DecodeRichTreeBase64(unescaped);
    TRichTreePtr richTree = DeserializeRichTree(packedTree);

    out << "digraph G {\n";
    out << "graph [fontname = \"sans serif\"];\n";
    out << "node [fontname = \"sans serif\"];\n";
    out << "edge [fontname = \"sans serif\"];\n";
    out << "ordering = out;" << Endl;

    DeclareNodes(richTree, out, encoding);
    DeclareLinks(richTree, out, encoding);

    out << "softness [label = \"Softness: " << ToString(richTree->Softness) << "\", shape=\"plaintext\", labelloc=\"bottom\", labeljust=\"right\"];" << Endl;

    out << "}" << Endl;
}

void GenerateGraphViz (IInputStream& in, IOutputStream& out, ECharset encoding)
{
    TString line;
    while (in.ReadLine(line)) {
        GenerateGraphViz(line, out, encoding);
    }
}

}
