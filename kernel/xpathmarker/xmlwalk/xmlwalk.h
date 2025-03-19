#pragma once

#include <libxml/tree.h>

#include <util/generic/string.h>

namespace NHtml {
namespace NXmlWalk {

inline bool IsTextNode(const xmlNodePtr node)
{
    return node != nullptr && node->type == XML_TEXT_NODE && node->content != nullptr;
}

inline bool IsSingleChild(const xmlNodePtr node)
{
    return node != nullptr && node->next == nullptr && node->parent != nullptr && node->parent->children == node;
}

// Go up by parents until root is reached or a next sibling is found,
// in the latter case move to the sibling.
// Requires @root to be an ancestor of @node.
inline xmlNodePtr CrawlUpNext(xmlNodePtr node, xmlNodePtr root)
{
    while (node != root && node->next == nullptr) {
        node = node->parent;
    }

    return (node == root) ? nullptr : node->next;
}

inline xmlNodePtr NextNodeInSubtree(xmlNodePtr node, const xmlNodePtr root, bool descentAllowed = true)
{
    return
        node == nullptr                             ? nullptr               :
        descentAllowed && node->children != nullptr ? node->children     :
                                                   CrawlUpNext(node, root);
}

TString GetNodeTextContent(xmlNodePtr node);

} // namespace NXmlWalk
} // namespace NHtml

