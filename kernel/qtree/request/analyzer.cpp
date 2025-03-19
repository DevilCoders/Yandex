#include <kernel/search_daemon_iface/reqtypes.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>

#include "nodebase.h"
#include "req_node.h"
#include "analyzer.h"

namespace {

    struct TNodeTraits {
        static bool IsNumberNode(const TRequestNode* node) {
            const TTokenStructure& tokens = node->GetSubTokens();
            return !tokens.empty() && (tokens[0].Type == TOKEN_NUMBER || tokens[0].Type == TOKEN_FLOAT);
        }

        static bool NodeAllowsMinusOp(const TRequestNode* node) {
            return ((IsWordOrMultitoken(*node) && !IsNumberNode(node))
                || (node->Parens && !IsZone(*node) && node->Op() != oRestrictByPos) // exclude: [-(title:(a b))] and [a -(b inpos:1..2)]
                || node->IsQuoted());
        }

        static bool IsBinaryOp(const TRequestNode* n) {
            return (
                (n->Op() == oRefine ||
                n->Op() == oOr ||
                n->Op() == oWeakOr ||
                n->Op() == oRestrDoc ||
                n->Op() == oAndNot ||
                IsAndOp(*n)) &&
                !n->Parens && !IsMultitoken(*n));
        }
    };

    struct TLeftToRight {
        static TRequestNode* GetFirst(TRequestNode* node) {
            return node->Left;
        }
        static TRequestNode* GetSecond(TRequestNode* node) {
            return node->Right;
        }
    };

    struct TRightToLeft {
        static TRequestNode* GetFirst(TRequestNode* node) {
            return node->Right;
        }
        static TRequestNode* GetSecond(TRequestNode* node) {
            return node->Left;
        }
    };

    //! @attention this function must NOT throw exceptions
    void SwapRoots(TRequestNode* newRoot, TRequestNode* oldRoot) {
        newRoot->GC.Swap(oldRoot->GC);

        ui32 softness = oldRoot->GetSoftness();
        oldRoot->SetSoftness(0);
        newRoot->SetSoftness(softness);
    }

    struct TNoContext {
    };

    //! an auxiliary class that walks a request binary tree - nodes have maximum two children: left and rigth
    //! @note prototype of walking function
    //! @code
    //! class TDerived : private TReqTreeWalker<TLeftToRight> {
    //!     typedef TReqTreeWalker<TLeftToRight> TWalker;
    //! public:
    //!     void WalkTree(TRequestNode* root) {
    //!         TWalker::TParents parents;
    //!         TRequestNode* node = root;
    //!         while (node) {
    //!             ///////////////////////////////////////////////////
    //!             // do something with node
    //!             ///////////////////////////////////////////////////
    //!             node = GetNextNode(node, parents); // it can return NULL
    //!         }
    //!     }
    //! };
    //! @endcode
    template <typename TDirection, typename TContext = TNoContext>
    class TReqTreeWalker {
    public:
        struct TParent {
            TRequestNode* Node;
            int CurChild;           //!< index of child node being processed: 0 - Left, 1 - Right
            TContext Context;
        };

        //! allows insertion into the front, iteration and deletion
        //! @note it can be replaced with @c std::deque
        class TParents : private TNonCopyable {
            typedef TVector<TParent> TMyVector;
            TMyVector Vec;
        public:
            typedef typename TMyVector::reverse_iterator TIterator;

            TParents() {
                Vec.reserve(128);
            }
            TIterator Begin() {
                return Vec.rbegin();
            }
            TIterator End() {
                return Vec.rend();
            }
            void Clear() {
                Vec.clear();
            }
            void PushFront(const TParent& parent) {
                Vec.push_back(parent);
            }
            void Erase(TIterator first, TIterator last) {
                Vec.erase(last.base(), first.base());
            }
            bool IsEmpty() const {
                return Vec.empty();
            }
            size_t GetSize() const {
                return Vec.size();
            }
        };

        typedef typename TParents::TIterator TParentIterator;

        //! returns the next node in the tree when request tree is walked from the left to the right
        static TRequestNode* GetNextNode(TRequestNode* node, TParents& parents) {
            if (TDirection::GetFirst(node)) {
                const TParent parent = { node, 0, TContext() };
                parents.PushFront(parent);
                return TDirection::GetFirst(node);
            } else if (TDirection::GetSecond(node)) {
                const TParent parent = { node, 1, TContext() };
                parents.PushFront(parent);
                return TDirection::GetSecond(node);
            }
            return FindNextNode(parents); // it can return NULL
        }

        //! finds the next node and removes all processed parent nodes from the collection
        //! @param parents      collection of parent nodes
        //! @param first        the function starts to search next node from this parent node
        static TRequestNode* FindNextNode(TParents& parents) {
            TParentIterator it = parents.Begin();
            while (it != parents.End() && (it->CurChild == 1 || !TDirection::GetSecond(it->Node)))
                ++it;
            if (it == parents.End()) {
                parents.Clear();
                return nullptr;
            }
            it->CurChild = 1;
            Y_ASSERT(TDirection::GetSecond(it->Node));
            TRequestNode* const node = TDirection::GetSecond(it->Node);
            parents.Erase(parents.Begin(), it);
            return node;
        }

        template <typename TNodeHandler>
        void WalkTree(TRequestNode* root, TNodeHandler& handler, TContext ctx = TContext()) {
            if (TDirection::GetFirst(root)) {
                // if it returns false left and right subtrees will not be processed
                if (handler.OnParent(root, 0, ctx)) {
                    WalkTree(TDirection::GetFirst(root), handler, ctx);
                } else {
                    return;
                }
            }
            if (TDirection::GetSecond(root)) {
                // if it returns false left or right (depends on LeftToRight/RightToLeft) subtree will not be processed
                if (handler.OnParent(root, 1, ctx)) {
                    WalkTree(TDirection::GetSecond(root), handler, ctx);
                } else {
                    return;
                }
            }
            if (!TDirection::GetFirst(root) && !TDirection::GetSecond(root)) {
                handler.OnLeaf(root, ctx);
            }
        }
    };

    class TMinusOpHandler : public TReqTreeWalker<TRightToLeft> {
    public:

        //! for example: zone, expr. in parentheses, quoted expr., multitoken
        static bool IsGroupingNode(const TRequestNode* node) {
            // all grouping nodes except oRestrictByPos can have Necessity == nMUSTNOT
            return IsZone(*node)
                || node->Op() == oRestrictByPos
                || node->Parens
                || node->IsQuoted()
                || IsMultitoken(*node);
        }

        //! for example: word, attribute, attribute interval
        static bool IsTerminalNode(const TRequestNode* node) {
            return IsWordOrMultitoken(*node) || IsAttribute(*node);
        }

        static void ResetMinusOp(TRequestNode* node) {
            if (node->Necessity == nMUSTNOT)
                node->Necessity = nDEFAULT;
        }

        //! resets Necessity nMUSTNOT for all nodes including @c root
        static void ResetMinusOps(TRequestNode* root) {
            TParents parents;
            TRequestNode* node = root;
            while (node) {
                // don't choose terminal or grouping nodes...
                ResetMinusOp(node);
                node = GetNextNode(node, parents);
            }
        }

        static void ResetChildrenMinusOps(TRequestNode* node) {
            if (node->Left)
                ResetMinusOps(node->Left);
            if (node->Right)
                ResetMinusOps(node->Right);
        }

        //! returns @c true if minus op is allowed for children
        static bool ParentAllowsMinusOp(const TRequestNode* parent) {
            Y_ASSERT(parent);
            return ((IsAndOp(*parent) && parent->GetLevel() == 2) || parent->GetPhraseType() == PHRASE_PHRASE);
        }

        static void ChangeParent(TRequestNode* parent) {
            if (parent->GetPhraseType() == PHRASE_PHRASE) {
                parent->SetOpInfo(DocumentPhraseOpInfo); // just in case...
            }
            parent->OpInfo.Op = oAndNot;
            parent->SetPhraseType(PHRASE_USEROP);
        }

        //! returns @c true if node is oRefine, oOr, oWeakOr, oRestrDoc or oAndNot operator
        //! @note child nodes of operators <-, |, ^, ~~ are stayed under these operators
        static bool IsNewParent(const TRequestNode* node) {
            // see precedence of the operators in req_pars.y
            return node->Op() == oRefine || node->Op() == oOr || node->Op() == oWeakOr || node->Op() == oRestrDoc || node->Op() == oAndNot;
        }

        static TRequestNode* MoveNodeWithParent(const TParentIterator parent, const TParentIterator grandparent, TParents& parents, THolder<TRequestNode>& root) {
            Y_ASSERT(parent != parents.End() && parent + 1 == grandparent);

            // find the new parent
            // since parent node is oAnd - new parent should be oRefine, oRestrDoc, oOr, oWeakOr or oAndNot
            // there are no any oZone, oRestrictByPos nodes above (also there should not be () "" and multitokens)

            TParentIterator it = grandparent;
            while (it != parents.End() && !IsNewParent(it->Node))
                ++it;

            Y_ASSERT(parent->CurChild == 0); // 'node' must always be the right node of 'parent' otherwise parent is already changed

            if (grandparent->CurChild == 0) {
                grandparent->Node->Right = parent->Node->Left;
            } else {
                Y_ASSERT(grandparent->CurChild == 1);
                grandparent->Node->Left = parent->Node->Left;
            }

            TRequestNode* const nextNode = parent->Node->Left;

            if (it == parents.End()) {
                // move to the root
                parent->Node->Left = root.Get();
                SwapRoots(parent->Node, root.Release());
                root.Reset(parent->Node);
            } else {
                // move under new parent
                if (it->CurChild == 0) {
                    parent->Node->Left = it->Node->Right;
                    it->Node->Right = parent->Node;
                } else {
                    Y_ASSERT(it->CurChild == 1);
                    parent->Node->Left = it->Node->Left;
                    it->Node->Left = parent->Node;
                }
            }

            return nextNode;
        }

        static TParentIterator FindLeftOp(TParents& parents) {
            TParentIterator it;
            for (it = parents.Begin(); it != parents.End(); ++it) {
                if (it->CurChild == 0) // parentheses are not taken into account
                    break;
            }
            return it;
        }

        static bool IsLeftmostNode(TParents& parents) {
            const TParentIterator leftOp = FindLeftOp(parents);
            if (leftOp == parents.End())
                return true;
            const TRequestNode* const node = leftOp->Node;
            // node is an operator joined subrequests
            return (node->Op() == oRefine || node->Op() == oOr || node->Op() == oWeakOr || node->Op() == oRestrDoc); // see IsNewParent()...
        }

        //! @attention this function supposes that all minus ops are valid - RemoveInvalidMinusOps was called
        TRequestNode* ProcessMinusOps(TRequestNode* root) {
            THolder<TRequestNode> holder(root);
            TParents parents;
            TRequestNode* node = root;

            // walking tree from right to left
            while (node) {
                ///////////////////////////////////////////////////

                if (node->Necessity == nMUSTNOT) {
                    // verify whether node is leftmost: [a
                    if (!IsLeftmostNode(parents)) {
                        const TParentIterator parent = parents.Begin();
                        // minus op is allowed for oAnd level 2 only
                        Y_ASSERT(TNodeTraits::NodeAllowsMinusOp(node));// && parent != parents.End() && ParentAllowsMinusOp(parent->Node));
                        Y_ASSERT(node->Op() != oRestrictByPos); // oRestrictByPos node cannot have Necessity == nMUSTNOT

                        if (!parents.IsEmpty()) {
                            const TParentIterator grandparent = parent + 1;

                            if (grandparent == parents.End() || IsNewParent(grandparent->Node)) {
                                // the parent can be changed to oAndNot without moving
                                ChangeParent(parent->Node);
                                node->Necessity = nDEFAULT;
                            } else {
                                TRequestNode* const nextNode = MoveNodeWithParent(parent, grandparent, parents, holder);

                                ChangeParent(parent->Node);
                                node->Necessity = nDEFAULT;

                                ResetChildrenMinusOps(node);

                                parents.Erase(parents.Begin(), grandparent);
                                node = nextNode;
                                continue;
                            }
                        }
                    }
                }

                ///////////////////////////////////////////////////
                node = GetNextNode(node, parents); // it can return NULL
            }

            return holder.Release();
        }

    };

    struct TRemovePhraseContext {
        bool RemovePhrase;

        TRemovePhraseContext()
            : RemovePhrase(false)
        {
        }
    };

    //! changes PHRASE_PHRASE to PHRASE_USEROP for all nodes of the right subtree of operator ~~
    //! @note tree must be walked from the left to the right
    struct TRemovePhraseHandler {
        //! @param context      it can be changed in this function
        bool OnParent(TRequestNode* node, int curChild, TRemovePhraseContext& context) {
            if (context.RemovePhrase) {
                if (node->GetPhraseType() == PHRASE_PHRASE) {
                    node->SetOpInfo(DocumentPhraseOpInfo);
                    node->SetPhraseType(PHRASE_USEROP);
                }
            } else if (node->Op() == oAndNot && node->GetLevel() == 2 && curChild == 1)
                context.RemovePhrase = true;
            return true;
        }
        void OnLeaf(TRequestNode* /*node*/, const TRemovePhraseContext& /*context*/) {
            // do nothing
        }
    };

    struct TRemoveMinusOpContext {
        const TRequestNode* Parent;

        TRemoveMinusOpContext()
            : Parent(nullptr)
        {
        }
    };

    struct TRemoveMinusOpHandler {
        bool MinusAllowed;
        TRequestNode* LeftmostNode; //!< it can be leaf of grouping node

        TRemoveMinusOpHandler()
            : MinusAllowed(true)
            , LeftmostNode(nullptr)
        {
        }

        ~TRemoveMinusOpHandler() {
            if (LeftmostNode)
                TMinusOpHandler::ResetMinusOp(LeftmostNode);
        }

        //! @param context      contains pointer to parent node at the beginning of this function and changes the pointer
        //!                     before exit if curChild == 0
        //! @return true if children of the parent should be processed
        bool OnParent(TRequestNode* node, int curChild, TRemoveMinusOpContext& context) {
            if (TMinusOpHandler::IsNewParent(node) && curChild == 1) { // going to the left subtree
                MinusAllowed = true;
                if (LeftmostNode) // resets minus op of the 'c' node: [a -b | -c -d] -> [a -b | c -d]
                    TMinusOpHandler::ResetMinusOp(LeftmostNode);
            } else if (context.Parent != node) { // means that node is already processed, (curChild == 0) can't be here because zone [-title:(attr:a)] can have only left subtree
                if (TMinusOpHandler::IsGroupingNode(node)) {
                    ProcessNode(node, context.Parent);
                    TMinusOpHandler::ResetChildrenMinusOps(node);
                    LeftmostNode = node;
                    return false; // don't process children
                } else
                    Y_ASSERT(node->Necessity == nDEFAULT);

                context.Parent = node; // change parent for child nodes
            }

            return true;
        }

        //! called for terminal nodes only
        void OnLeaf(TRequestNode* node, const TRemoveMinusOpContext& context) {
            ProcessNode(node, context.Parent);
            LeftmostNode = node;
        }

    private:
        void ProcessNode(TRequestNode* node, const TRequestNode* parent) {
            Y_ASSERT(TMinusOpHandler::IsGroupingNode(node) || TMinusOpHandler::IsTerminalNode(node));

            if (!MinusAllowed)
                TMinusOpHandler::ResetMinusOp(node);
            else {
                if (node->Necessity == nMUSTNOT) {
                    if (!TNodeTraits::NodeAllowsMinusOp(node) || (parent && !TMinusOpHandler::ParentAllowsMinusOp(parent))) {
                        // minus of the 'a' and 'b' nodes are reset here: [-a | -b -c] -> [a | b -c]
                        TMinusOpHandler::ResetMinusOp(node);
                        MinusAllowed = false;
                    }
                } else
                    MinusAllowed = false;
            }
        }
    };

    //! assignes values to WildCard properties
    class TWildCardOpHandler : public TReqTreeWalker<TLeftToRight> {
    public:
        TRequestNode*  ProcessWildCards(TRequestNode* root) {
            TRequestNode* node = root;
            TParents parents;
            while (node) {
                EWildCardFlags wc = WILDCARD_NONE;
                if (!!node->GetTextBefore() && *(node->GetTextBefore().end()-1) == '*') {
                    wc = EWildCardFlags(wc | WILDCARD_SUFFIX);
                    node->FormType = fExactWord;
                }
                if (*(node->GetTextAfter().begin()) == '*') {
                    wc = EWildCardFlags(wc | WILDCARD_PREFIX);
                    node->FormType = fExactWord;
                }
                node->SetWildCard(wc);
                node = GetNextNode(node, parents);
            }
            return root;
        }
    };

} // namespace

TRequestNode* TRequestAnalyzer::ProcessMinusOps(TRequestNode* root) {
    TMinusOpHandler handler;
    return handler.ProcessMinusOps(root);
}

TRequestNode* TRequestAnalyzer::ProcessWildCards(TRequestNode* root) {
    TWildCardOpHandler handler;
    return handler.ProcessWildCards(root);
}

void TRequestAnalyzer::PrepareTree(TRequestNode* root) {
    {
        TReqTreeWalker<TLeftToRight, TRemovePhraseContext> walker;
        TRemovePhraseHandler handler;
        walker.WalkTree(root, handler);
    }
    {
        TReqTreeWalker<TRightToLeft, TRemoveMinusOpContext> walker;
        TRemoveMinusOpHandler handler;
        walker.WalkTree(root, handler);
    }
}

TRequestNode* TRequestAnalyzer::ProcessTree(TRequestNode* root, ui32 requestProcessingFlags) {
    // parentheses at the root node are required for wizard rules because
    // they often call to CreateRichTree() for parts and then compose the request
    bool setParens = false;
    if (root->Parens && root->FormType == fGeneral && root->Necessity == nDEFAULT) {
        setParens = true;
        root->Parens = false;
    }

    THolder<TRequestNode> holder(root);
    PrepareTree(holder.Get());

    holder.Reset(ProcessMinusOps(holder.Release()));
    if (requestProcessingFlags & PRF_WILDCARDS) {
        ProcessWildCards(holder.Get());
    }

    if (setParens)
        holder->Parens = true;

    return holder.Release();
}
