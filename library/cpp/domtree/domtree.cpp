#include "domtree.h"
#include "treedata.h"
#include "treetext_impl.h"

#include <util/stream/output.h>

namespace NDomTree {
    class TDomAttr: public IDomAttr {
    private:
        const ITreeDataAccessor& TreeData;
        TAttrData Data;

    public:
        TDomAttr(const ITreeDataAccessor& treeData, const TAttrData& data)
            : TreeData(treeData)
            , Data(data)
        {
        }
        TStringBuf Name() const override {
            return TreeData.CText(Data.Name);
        }
        TStringBuf Value() const override {
            return TreeData.CText(Data.Value);
        }
        bool IsBoolean() const override {
            return Data.Boolean;
        }
        ui64 Line() const override {
            return Data.Line;
        }
        ui64 Column() const override {
            return Data.Column;
        }

        friend class TDomTree;
    };

    class TDomNode: public IDomNode, public INodeText {
    private:
        using TAttrPtr = const IDomAttr*;

        const ITreeDataAccessor& TreeData;
        TNodeData NodeData;

    public:
        TDomNode(const ITreeDataAccessor& treeData, const TNodeData& node)
            : TreeData(treeData)
            , NodeData(node)
        {
        }

        // node properties
        ui32 Id() const override {
            return NodeData.Id;
        }
        EDomNodeType Type() const override {
            return NodeData.Type;
        }
        bool IsElement() const override {
            return EDomNodeType::NT_Element == NodeData.Type;
        }
        bool IsText() const override {
            return EDomNodeType::NT_Text == NodeData.Type;
        }
        HT_TAG Tag() const override {
            return NodeData.HtmlTag;
        }
        TStringBuf TagName() const override {
            return TreeData.CText(NodeData.TagName);
        };
        TEXT_WEIGHT Weight() const override {
            return NodeData.Weight;
        }
        TWtringBuf Text() const override {
            return TreeData.WText(NodeData.Text);
        }
        TPosting PosBeg() const override {
            return NodeData.Beg;
        }
        TPosting PosEnd() const override {
            return NodeData.End;
        }
        ui64 Line() const override {
            return NodeData.Line;
        }
        ui64 Column() const override {
            return NodeData.Column;
        }

        // attrs
        size_t AttrCount() const override {
            return NodeData.AttrCnt;
        }

        const IDomAttr* GetAttr(size_t index) const override {
            Y_ASSERT(index < NodeData.AttrCnt);
            if (index >= NodeData.AttrCnt) {
                return nullptr;
            }
            return TreeData.GetAttr(NodeData.AttrBeg + index);
        }

        bool HasAttr(const TStringBuf& attrName) const override {
            size_t n = AttrCount();
            for (size_t i = 0; i < n; ++i) {
                if (GetAttr(i)->Name() == attrName) {
                    return true;
                }
            }
            return false;
        }

        TStringBuf Attr(const TStringBuf& attrName) const override {
            size_t n = AttrCount();
            for (size_t i = 0; i < n; ++i) {
                const auto attr = GetAttr(i);
                if (attr->Name() == attrName) {
                    return attr->Value();
                }
            }
            return TStringBuf();
        }

        // siblings
        const IDomNode* Next() const override {
            if (NodeData.NextSibling == 0) {
                return nullptr;
            }
            return TreeData.GetNode(NodeData.NextSibling);
        }
        const IDomNode* Prev() const override {
            if (NodeData.PrevSibling == 0) {
                return nullptr;
            }
            return TreeData.GetNode(NodeData.PrevSibling);
        }
        const IDomNode* Parent() const override {
            if (TreeData.GetNode(NodeData.Parent) == this) {
                Y_ASSERT(NodeData.Parent == 0);
                return nullptr;
            }
            return TreeData.GetNode(NodeData.Parent);
        }

        // children
        size_t ChildCount() const override {
            return NodeData.ChildCnt;
        }
        const IDomNode* GetChild(size_t idx) const override {
            const IDomNode* res = FirstChild();
            for (size_t i = 0; i < idx; ++i)
                res = res->Next();
            return res;
        }
        const IDomNode* FirstChild() const override {
            if (NodeData.ChildBeg == 0) {
                return nullptr;
            }
            return TreeData.GetNode(NodeData.ChildBeg);
        }

        // text
        const INodeText* NodeText() const override {
            return this;
        }

        TWtringBuf RawTextNormal() const override {
            return RawText(TreeData, NodeData.FirstToken, NodeData.LastToken);
        }

        TTextChunkIterator RawTextAll() const override {
            return ChunkIterator(TreeData, NodeData.FirstToken, NodeData.LastToken);
        }

        TSentTextIterator Sents() const override {
            // temporarily switch off
            return SentIterator(TreeData, TreeData.TokenCount(), TreeData.TokenCount());
            //        return SentIterator(TreeData, NodeData.FirstToken, NodeData.LastToken);
        }

        TTokenIterator Tokens(bool noindex) const override {
            // temporarily switch off
            Y_ASSERT(NodeData.LastToken <= TreeData.TokenCount());
            auto begin = TreeData.TokenDataBegin() + TreeData.TokenCount(); // NodeData.FirstToken;
            auto end = TreeData.TokenDataBegin() + TreeData.TokenCount();   //NodeData.LastToken;
            return TokenIterator(TreeData, begin, end, noindex);
        }

        TTokenIterator RTokens(bool noindex) const override {
            // temporarily switch off
            Y_ASSERT(NodeData.LastToken <= TreeData.TokenCount());
            TTokenDataReverseIterator begin(TreeData.TokenDataBegin() + TreeData.TokenCount() /* NodeData.LastToken*/);
            TTokenDataReverseIterator end(TreeData.TokenDataBegin() + TreeData.TokenCount() /* NodeData.FirstToken */);
            return TokenIterator(TreeData, begin, end, noindex);
        }

        ~TDomNode() override = default;

        friend class TDomTree;
    };

    class TDomTree: public ITreeDataAccessor, public ITreeDataModifier {
    private:
        using TDomNodes = TVector<TDomNode>;

        TTreeData TreeData;
        TDomNodes Nodes;
        TVector<TDomAttr> Attrs;
        TVector<TTokenData> Tokens;

        class TPreorderTraverser: public INodeTraverser {
        private:
            TDomNodes::const_iterator Current;
            TDomNodes::const_iterator End;

        public:
            TPreorderTraverser(const TDomNodes& nodes)
                : Current(nodes.begin())
                , End(nodes.end())
            {
            }

            const IDomNode* Next() override {
                if (Current == End) {
                    return nullptr;
                }
                return &(*Current++);
            }
        };

    public:
        TDomTree()
            : TreeData()
            , Nodes()
            , Attrs()
            , Tokens()
        {
        }

        ~TDomTree() override = default;

        const IDomNode* GetRoot() const override {
            if (Nodes.empty()) {
                return nullptr;
            }
            return &Nodes[0];
        }

        TNodeIterator PreorderTraversal() const override {
            return TNodeIterator(TSimpleSharedPtr<INodeTraverser>(new TPreorderTraverser(Nodes)));
        }

        // ITreeData
        const IDomNode* GetNode(ui32 id) const override {
            Y_ASSERT(id < Nodes.size());
            return &Nodes[id];
        }
        const IDomAttr* GetAttr(ui32 id) const override {
            Y_ASSERT(id < Attrs.size());
            return &Attrs[id];
        }
        TStringBuf CText(const TStringPiece& p) const override {
            if (p.Src == TStringPiece::SPS_ATTR) {
                Y_ASSERT(p.Start + p.Len <= TreeData.AttrBuf.size());
                return TStringBuf(TreeData.AttrBuf.begin() + p.Start, p.Len);
            }
            return TStringBuf();
        }
        TWtringBuf WText(const TStringPiece& p) const override {
            if (p.Src == TStringPiece::SPS_TEXT) {
                Y_ASSERT(p.Start + p.Len <= TreeData.TextBuf.size());
                return TWtringBuf(TreeData.TextBuf.begin() + p.Start, p.Len);
            } else if (p.Src == TStringPiece::SPS_NOINDEX) {
                Y_ASSERT(p.Start + p.Len <= TreeData.NoindexBuf.size());
                return TWtringBuf(TreeData.NoindexBuf.begin() + p.Start, p.Len);
            }
            return TWtringBuf();
        }
        ui32 TokenCount() const override {
            return Tokens.size();
        }
        const TTokenData& TokenData(ui32 id) const override {
            Y_ASSERT(id < Tokens.size());
            return Tokens[id];
        }
        // ITreeDataBuilder
        ui32 NodeCount() const override {
            return Nodes.size();
        }
        ui32 AttrCount() const override {
            return Attrs.size();
        }
        TNodeData& NodeData(ui32 id) override {
            Y_ASSERT(id < Nodes.size());
            return Nodes[id].NodeData;
        }
        TAttrData& AttrData(ui32 id) override {
            Y_ASSERT(id < Attrs.size());
            return Attrs[id].Data;
        }
        TTokenData& TokenData(ui32 id) override {
            Y_ASSERT(id < Tokens.size());
            return Tokens[id];
        }
        TTokenDataIterator TokenDataBegin() const override {
            return Tokens.begin();
        }
        TTokenDataIterator TokenDataEnd() const override {
            return Tokens.end();
        }
        TNodeData& PushNodeData(const TNodeData& node) override {
            Nodes.push_back(TDomNode(*this, node));
            return Nodes.back().NodeData;
        }
        void PushAttrData(const TAttrData& attr) override {
            Attrs.push_back(TDomAttr(*this, attr));
        }
        void PushTokenData(const TTokenData& token) override {
            Tokens.push_back(token);
        }
        TString& AttrBuf() override {
            return TreeData.AttrBuf;
        }
        TUtf16String& TextBuf() override {
            return TreeData.TextBuf;
        }
        TUtf16String& NoindexBuf() override {
            return TreeData.NoindexBuf;
        }
    };

    TTreeBuilderPtr CreateTreeBuilder() {
        return TTreeBuilderPtr(new TDomTree());
    };

}
