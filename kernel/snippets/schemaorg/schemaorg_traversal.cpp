#include "schemaorg_traversal.h"
#include <kernel/snippets/schemaorg/proto/schemaorg.pb.h>
#include <util/stream/output.h>

namespace NSchemaOrg {
    const TTreeNode* FindFirstItemprop(const TTreeNode* node, const TString& name) {
        if (node) {
            for (const TTreeNode& child : node->GetNode()) {
                if (IsPropertyName(child, name)) {
                    return &child;
                }
            }
        }
        return nullptr;
    }

    TList<const TTreeNode*> FindAllItemprops(const TTreeNode* node, const TString& name, bool tryPluralName) {
        TList<const TTreeNode*> res;
        if (node) {
            TString pluralName = tryPluralName ? name + "s" : TString();
            for (const TTreeNode& child : node->GetNode()) {
                if (IsPropertyName(child, name) || (tryPluralName && IsPropertyName(child, pluralName))) {
                    res.push_back(&child);
                }
            }
        }
        return res;
    }

    TList<const TTreeNode*> ReplaceItemtypesWithNames(const TList<const TTreeNode*>& nodes) {
        TList<const TTreeNode*> res;
        for (const TTreeNode* node : nodes) {
            TList<const TTreeNode*> nameNodes;
            if (node->ItemtypesSize()) {
                nameNodes = FindAllItemprops(node, "name", false);
            }
            if (!nameNodes.empty()) {
                res.insert(res.end(), nameNodes.begin(), nameNodes.end());
            } else {
                res.push_back(node);
            }
        }
        return res;
    }

    const TTreeNode* FindSingleItemprop(const TTreeNode* node, const TString& name) {
        const TTreeNode* resultNode = nullptr;
        if (node) {
            for (const TTreeNode& child : node->GetNode()) {
                if (IsPropertyName(child, name)) {
                    if (resultNode) {
                        return nullptr;
                    }
                    resultNode = &child;
                }
            }
        }
        return resultNode;
    }

    const TTreeNode* FindSingleItemtype(const TTreeNode* node, const TString& name) {
        const TTreeNode* resultNode = nullptr;
        if (node) {
            for (const TTreeNode& child : node->GetNode()) {
                if (IsItemOfType(child, name)) {
                    if (resultNode) {
                        return nullptr;
                    }
                    resultNode = &child;
                }
            }
        }
        return resultNode;
    }

    bool IsItemOfType(const TTreeNode& node, const TStringBuf& name) {
        for (const auto& itemType : node.GetItemtypes()) {
            if (itemType == name) {
                return true;
            }
        }
        return false;
    }

    bool IsPropertyName(const TTreeNode& node, const TStringBuf& name) {
        for (const auto& prop : node.GetItemprops()) {
            if (prop == name) {
                return true;
            }
        }
        return false;
    }

    bool TraverseTree(const TTreeNode* root, const TNodeChecker& nodeChecker) {
        if (root) {
            for (const TTreeNode& child : root->GetNode()) {
                if (!nodeChecker(child) || !TraverseTree(&child, nodeChecker)) {
                    return false;
                }
            }
        }
        return true;
    }

} // namespace NSchemaOrg
