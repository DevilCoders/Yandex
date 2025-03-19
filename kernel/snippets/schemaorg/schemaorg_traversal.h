#pragma once

#include <util/generic/string.h>
#include <util/generic/list.h>
#include <util/generic/strbuf.h>

#include <functional>

namespace NSchemaOrg {
    class TTreeNode;

    const TTreeNode* FindFirstItemprop(const TTreeNode* node, const TString& name);
    const TTreeNode* FindSingleItemprop(const TTreeNode* node, const TString& name);
    TList<const TTreeNode*> FindAllItemprops(const TTreeNode* node, const TString& name, bool tryPluralName);
    TList<const TTreeNode*> ReplaceItemtypesWithNames(const TList<const TTreeNode*>& nodes);
    const TTreeNode* FindSingleItemtype(const TTreeNode* node, const TString& name);

    bool IsItemOfType(const TTreeNode& node, const TStringBuf& name);
    bool IsPropertyName(const TTreeNode& node, const TStringBuf& name);

    using TNodeChecker = std::function<bool(const TTreeNode& node)>;
    bool TraverseTree(const TTreeNode* root, const TNodeChecker& nodeChecker);

} // namespace NSchemaOrg
