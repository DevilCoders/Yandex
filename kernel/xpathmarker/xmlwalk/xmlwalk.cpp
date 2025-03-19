#include "xmlwalk.h"

#include <library/cpp/html/entity/htmlentity.h>

#include <cstring>

namespace NHtml {
namespace NXmlWalk {

namespace {
    static const char* SCRIPT_TAG = "script";

    inline bool IsScriptNode(const xmlNodePtr node)
    {
        return node != nullptr && strcmp((const char*)node->name, SCRIPT_TAG) == 0;
    }

    xmlNodePtr NextNonscriptTextNodeInSubtree(xmlNodePtr node, const xmlNodePtr root)
    {
        if (IsTextNode(node)) {
            node = NextNodeInSubtree(node, root);
        }

        while (node != nullptr) {
            if (IsScriptNode(node)) {
                node = NextNodeInSubtree(node, root, false);
            } else if (! IsTextNode(node)) {
                node = NextNodeInSubtree(node, root);
            } else {
                break;
            }
        }
        return node;
    }
}

TString GetNodeTextContent(xmlNodePtr node)
{
    TString result;

    const xmlNodePtr root = node;

    if (IsScriptNode(node) || ! IsTextNode(node)) {
        node = NextNonscriptTextNodeInSubtree(node, root);
    }

    while (node != nullptr) {
        if (! result.empty()) {
            result.push_back(' ');
        }
        result += (const char*)node->content;

        node = NextNonscriptTextNodeInSubtree(node, root);
    }

    return result;
}

} // namespace NXmlWalk
} // namespace NHtml
