#include <util/string/subst.h>
#include <util/stream/output.h>
#include <util/generic/deque.h>
#include "face.h"

namespace NHtmlTree {
    static TString Space(size_t len) {
        return TString(len, ' ');
    }

    static TString EventText(const THtmlChunk* e, bool qEscape = true) {
        if (!e)
            return "(no event)";
        if (!e->text || !e->leng)
            return "(empty)";
        TString s = TString(e->text, e->leng);
        if (!qEscape)
            return s;
        SubstGlobal(s, "\\", "\\\\");
        SubstGlobal(s, "\n", "\\n");
        SubstGlobal(s, "\"", "\\\"");
        return "\"" + s + "\"";
    }

    void DumpTreeNode(IOutputStream& os, const TNode* node, size_t indent);
    void DumpTreeNode(IOutputStream& os, const TTextNode* node, size_t indent);
    void DumpTreeNode(IOutputStream& os, const TElementNode* node, size_t indent);

    void DumpTreeNode(IOutputStream& os, const TNode* node, size_t indent) {
        if (node->Type == NODE_ELEMENT)
            DumpTreeNode(os, node->ToElementNode(), indent);
        else if (node->Type == NODE_TEXT)
            DumpTreeNode(os, node->ToTextNode(), indent);
        else {
            os << Space(indent) << (node->Type == NODE_IRREG ? 'I' : 'M') << ' '
               << EventText(node->Event) << Endl;
        }
    }

    void DumpTreeNode(IOutputStream& os, const TTextNode* node, size_t indent) {
        // clean tree - skip whitespace
        if (node->Event->IsWhitespace)
            return;

        os << Space(indent) << "T" << ' ' << EventText(node->Event) << Endl;
        /// @todo dump decoded text
    }

    void DumpTreeNode(IOutputStream& os, const TElementNode* node, size_t indent) {
        os << Space(indent) << "E" << ' ' << EventText(node->Event) << Endl;
        for (TNodeIter i = node->Begin(); i != node->End(); ++i)
            DumpTreeNode(os, *i, indent + 2);
    }

    void DumpTree(IOutputStream& os, const TTree* t) {
        os << "dumping tree..." << Endl;
        os << t->GetStorage()->MemoryAllocated() << " bytes used" << Endl;

        DumpTreeNode(os, t->RootNode(), 0);
    }

    using TNodePath = TDeque<TElementNode*>;
    using TNodePathIter = TDeque<TElementNode*>::const_iterator;

    void printPath(IOutputStream& os, const TNodePath& path, const char* outputDocPrefix) {
        if (outputDocPrefix) {
            os << outputDocPrefix;
        }

        for (auto i : path) {
            os << EventText(i->Event, false);
        }
    }

    void DumpTreeNodeXPath(IOutputStream& os, const TNode* node, TNodePath& path, const char* outputDocPrefix);

    void DumpTreeNodeXPathE(IOutputStream& os, const TElementNode* node, TNodePath& path, const char* outputDocPrefix) {
        TNodeIter i = node->Begin();
        if (i == node->End()) { // -- empty element
            printPath(os, path, outputDocPrefix);
            os << '\t' << "E" << '\t' << Endl;
        }
        for (; i != node->End(); ++i) {
            DumpTreeNodeXPath(os, *i, path, outputDocPrefix);
        }
    }

    void DumpTreeNodeXPathT(IOutputStream& os, const TTextNode* node, TNodePath& path, const char* outputDocPrefix) {
        // clean tree - skip whitespace
        if (node->Event->IsWhitespace)
            return;
        printPath(os, path, outputDocPrefix);
        os << '\t' << "T" << '\t' << EventText(node->Event, false) << Endl;
        /// @todo dump decoded text
    }

    void DumpTreeNodeXPath(IOutputStream& os, const TNode* node, TNodePath& path, const char* outputDocPrefix) {
        if (node->Type == NODE_ELEMENT) {
            path.push_back((TElementNode*)node->ToElementNode());
            DumpTreeNodeXPathE(os, (const TElementNode*)node->ToElementNode(), path, outputDocPrefix);
            path.pop_back();
        } else if (node->Type == NODE_TEXT) {
            DumpTreeNodeXPathT(os, node->ToTextNode(), path, outputDocPrefix);
        } else {
            printPath(os, path, outputDocPrefix);
            os << '\t' << (node->Type == NODE_IRREG ? 'I' : 'M');
            os << '\t' << EventText(node->Event, false) << Endl;
        }
    }

    void DumpXPaths(IOutputStream& os, const TTree* t, const char* outputDocPrefix) {
        TNodePath path;
        DumpTreeNodeXPath(os, t->RootNode(), path, outputDocPrefix);
    }

}
