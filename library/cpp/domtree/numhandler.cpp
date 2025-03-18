#include "numhandler.h"
#include "treedata.h"

#include <library/cpp/charset/wide.h>
#include <library/cpp/token/nlptypes.h>

#include <library/cpp/charset/codepage.h>
#include <library/cpp/charset/recyr.hh>
#include <library/cpp/html/entity/htmlentity.h>
#include <util/string/strip.h>
#include <util/memory/tempbuf.h>

namespace NDomTree {
    static const TUtf16String SPACE = u" ";

    static TUtf16String ProcessSpaces(const TWtringBuf& buf, bool stripLeft) {
        if (buf.empty()) {
            return TUtf16String();
        }
        bool leftSpace = IsSpace(*buf.begin());
        bool rightSpace = IsSpace(buf.back());
        if (buf.size() == 1 && leftSpace) {
            return SPACE;
        }
        TUtf16String res(buf);
        res = StripString(res);
        Collapse(res);
        if (res.empty()) {
            return SPACE;
        }
        if (leftSpace && !stripLeft) {
            res = SPACE + res;
        }
        if (rightSpace) {
            res.append(SPACE);
        }
        return res;
    }

    static void CharsetConvert(const TStringBuf& src, TString& dst, ECharset from, bool makeLower) {
        if (from != CODES_UTF8) {
            Recode(from, CODES_UTF8, src, dst);
        } else {
            dst = src;
        }

        if (makeLower) {
            if (IsStringASCII(dst.begin(), dst.end())) {
                const CodePage* cp = CodePageByCharset(CODES_ASCII);
                Y_ASSERT(nullptr != cp);
                ToLower(dst.begin(), dst.size(), *cp);
            } else {
                TUtf16String tmp = UTF8ToWide<true>(dst);
                tmp.to_lower();
                dst = WideToUTF8(tmp);
            }
        }
    }

    class TTreeBuilder {
    private:
        struct TTokenizedTextInfo {
            TStringPiece TextPos;
            ui32 FirstToken;
            bool Empty;

            TTokenizedTextInfo()
                : TextPos(TStringPiece::SPS_TEXT)
                , FirstToken(0)
                , Empty(true)
            {
            }

            void Update(ui32 start, ui32 len) {
                if (Empty) {
                    TextPos.Start = start;
                }
                Empty = false;
                TextPos.Len += len;
            }

            void Reset(ui32 ft) {
                TextPos.Start = 0;
                TextPos.Len = 0;
                FirstToken = ft;
                Empty = true;
            }
        };

        ui32 LinkNewNode() {
            ui32 nodeCnt = Builder.NodeCount();
            if (nodeCnt == 0) {
                return 0;
            }
            TNodeData& curParent = Builder.NodeData(CurrentParent);
            if (curParent.ChildBeg == 0) {
                curParent.ChildBeg = nodeCnt;
            }
            ui32 prev = curParent.ChildEnd;
            TNodeData& prevNode = Builder.NodeData(prev);
            if (prev > 0) {
                prevNode.NextSibling = nodeCnt;
            }
            curParent.ChildCnt++;
            curParent.ChildEnd = nodeCnt;
            return prev;
        }

        bool TextGoodParent() {
            HT_TAG tag = Builder.NodeData(CurrentParent).HtmlTag;
            switch (tag) {
                case HT_SCRIPT:
                    return ProcessScriptText;
                case HT_NOSCRIPT:
                case HT_STYLE:
                    return false;
                default:
                    break;
            }
            return true;
        }

        ITreeDataModifier& Builder;
        TPosting CurrentPos;
        ui32 CurrentParent;
        ui32 CurrentDepth;
        ui32 MaxDepth;
        TTokenizedTextInfo TokenizedText;
        bool TrimSpaces;
        ECharset Charset;
        TNlpInputDecoder Decoder;
        IPositionProvider* PositionProvider{nullptr};
        TStringBuf ParsedDocument;
        bool DecodeHtEntity{false};
        bool ProcessScriptText{false};

    public:
        TTreeBuilder(
            ITreeDataModifier& builder, ECharset charset, bool decode = false, bool processScriptText = false)
            : Builder(builder)
            , CurrentPos(0)
            , CurrentParent(0)
            , CurrentDepth(0)
            , MaxDepth(0)
            , TrimSpaces(true)
            , Charset(charset)
            , Decoder(charset)
            , DecodeHtEntity(decode)
            , ProcessScriptText(processScriptText)
        {
        }

        void SetPositionProvider(IPositionProvider* positionProvider) {
            PositionProvider = positionProvider;
        }

        void SetParsedDocument(TStringBuf document) {
            ParsedDocument = document;
        }

        void StartNode(const THtmlChunk& chunk, const TNumerStat& stat) {
            if (HT_PRE == chunk.Tag->id()) {
                TrimSpaces = false;
            }
            SetPosting(CurrentPos, stat.TokenPos.Break(), stat.TokenPos.Word(), MID_RELEV);
            ui32 prevSibling = LinkNewNode();
            ui32 newParent = Builder.NodeCount();
            TEXT_WEIGHT w = static_cast<TEXT_WEIGHT>(chunk.flags.weight);
            ui32 tagNameBeg = 0;
            ui32 tagNameLen = 0;
            if (chunk.Tag->id() == HT_any) {
                TString tagName;
                CharsetConvert(GetTagName(chunk), tagName, Charset, false);
                tagNameBeg = Builder.AttrBuf().size();
                tagNameLen = tagName.size();
                Builder.AttrBuf().append(tagName);
            }
            TNodeData& last = Builder.PushNodeData(
                TNodeData(
                    Builder.NodeCount(),
                    CurrentParent,
                    w,
                    chunk.Tag->id(),
                    TStringPiece(TStringPiece::SPS_ATTR, tagNameBeg, tagNameLen)
                )
            );
            CurrentParent = newParent;
            ++CurrentDepth;
            MaxDepth = Max(MaxDepth, CurrentDepth);
            last.AttrBeg = Builder.AttrCount();
            TString buf;
            for (ui32 i = 0; i < chunk.AttrCount; i++) {
                const NHtml::TAttribute& attr = chunk.Attrs[i];
                ui32 nameBeg = Builder.AttrBuf().size();
                CharsetConvert(TStringBuf(chunk.text + attr.Name.Start, attr.Name.Leng), buf, Charset, true);
                ui32 nameLen = buf.size();
                Builder.AttrBuf().append(buf);
                ui32 valBeg = Builder.AttrBuf().size();
                ui32 valLen = 0;
                if (!attr.IsBoolean()) {
                    if (DecodeHtEntity) {
                        TTempBuf tmp(attr.Value.Leng * 4);
                        size_t len = HtDecodeAttrToUtf8(
                            Charset, chunk.text + attr.Value.Start, attr.Value.Leng, tmp.Data(), tmp.Size());
                        buf = TString(tmp.Data(), len);
                    } else {
                        CharsetConvert(TStringBuf(chunk.text + attr.Value.Start, attr.Value.Leng), buf, Charset, false);
                    }
                    valLen = buf.size();
                }
                Builder.AttrBuf().append(buf);
                ui64 line = 0;
                ui64 column = 0;
                if (PositionProvider) {
                    PositionProvider->Position(chunk.text - ParsedDocument.data() + attr.Name.Start, line, column);
                }
                Builder.PushAttrData(TAttrData(newParent, TStringPiece(TStringPiece::SPS_ATTR, nameBeg, nameLen),
                                               TStringPiece(TStringPiece::SPS_ATTR, valBeg, valLen),
                                               attr.IsBoolean(), line, column));
            }

            last.AttrCnt = chunk.AttrCount;
            last.Beg = CurrentPos;
            last.PrevSibling = prevSibling;
            last.FirstToken = Builder.TokenCount();
            if (PositionProvider) {
                PositionProvider->Position(chunk.text - ParsedDocument.data(), last.Line, last.Column);
            }
            TokenizedText.Reset(last.FirstToken);
        }

        void UpdateLastToken(ui32 parent) {
            TNodeData& node = Builder.NodeData(parent);
            node.LastToken = Builder.TokenCount();
        }

        size_t GetMaxDepth() {
            return MaxDepth;
        }

        void StopNode(const THtmlChunk& chunk, const TNumerStat& stat) {
            if (HT_PRE == chunk.Tag->id()) {
                TrimSpaces = true;
            }
            SetPosting(CurrentPos, stat.TokenPos.Break(), stat.TokenPos.Word(), MID_RELEV);

            TNodeData& curParent = Builder.NodeData(CurrentParent);
            curParent.End = CurrentPos;
            UpdateLastToken(CurrentParent);
            CurrentParent = curParent.Parent;
            --CurrentDepth;
        }

        void PutText(const THtmlChunk& chunk, const TNumerStat& stat) {
            if (!TextGoodParent()) {
                return;
            }

            TTempArray<wchar16> buf(Decoder.GetDecodeBufferSize(chunk.leng));
            size_t encsz = Decoder.DecodeEvent(chunk, buf.Data());
            TWtringBuf txt(buf.Data(), encsz);

            bool add = false;
            ui32 firstToken = Builder.TokenCount();
            TStringPiece piece(TStringPiece::SPS_UNK);
            if (chunk.flags.weight == WEIGHT_ZERO) {
                piece = AppendNoindexText(txt);
                Builder.PushTokenData(TTokenData(CurrentParent, CurrentPos, piece, TOKEN_MIXED, TTokenData::TM_UNTOK));
                add = true;
            } else {
                piece = AppendText(txt);
                Builder.PushTokenData(TTokenData(CurrentParent, CurrentPos, piece, TOKEN_MIXED, TTokenData::TM_UNTOK));
                add = true;
            }

            if (add) {
                ui32 prevSibling = LinkNewNode();
                TNodeData node(Builder.NodeCount(), CurrentParent, static_cast<TEXT_WEIGHT>(chunk.flags.weight), piece);
                node.Beg = CurrentPos;
                SetPosting(CurrentPos, stat.TokenPos.Break(), stat.TokenPos.Word(), MID_RELEV);
                node.End = CurrentPos;
                node.PrevSibling = prevSibling;
                node.FirstToken = firstToken;
                node.LastToken = Builder.TokenCount();
                Builder.PushNodeData(node);
            }
        }

        // temporarily unused method
        void PutTextTokenized(const THtmlChunk& chunk, const TNumerStat& stat) {
            if (!TextGoodParent()) {
                return;
            }

            TStringPiece piece = TokenizedText.TextPos;
            bool add = false;
            if (chunk.flags.weight == WEIGHT_ZERO) {
                TUtf16String txt = CharToWide<true>(chunk.text, chunk.leng, Charset);
                if (TrimSpaces) {
                    txt = ProcessSpaces(txt, false);
                }
                if (!IsSpace(txt)) {
                    piece = AppendNoindexText(txt);
                    Builder.PushTokenData(TTokenData(CurrentParent, CurrentPos, piece, TOKEN_MIXED, TTokenData::TM_UNTOK));
                    add = true;
                }
            } else {
                piece = TokenizedText.TextPos;
                if (!IsSpace(TWtringBuf(Builder.TextBuf().begin() + piece.Start, piece.Len))) {
                    add = true;
                }
            }
            if (add) {
                ui32 prevSibling = LinkNewNode();
                TNodeData node(Builder.NodeCount(), CurrentParent, static_cast<TEXT_WEIGHT>(chunk.flags.weight), piece);
                node.Beg = CurrentPos;
                SetPosting(CurrentPos, stat.TokenPos.Break(), stat.TokenPos.Word(), MID_RELEV);
                node.End = CurrentPos;
                node.PrevSibling = prevSibling;
                node.FirstToken = TokenizedText.FirstToken;
                node.LastToken = Builder.TokenCount();
                Builder.PushNodeData(node);
            }
            TokenizedText.Reset(Builder.TokenCount());
        }

        TStringPiece AppendNoindexText(const TWtringBuf& buf) {
            if (Builder.NodeCount() == 0) {
                return TStringPiece(TStringPiece::SPS_NOINDEX);
            }
            ui32 beg = Builder.NoindexBuf().size();
            Builder.NoindexBuf().append(buf);
            ui32 len = Builder.NoindexBuf().size() - beg;
            return TStringPiece(TStringPiece::SPS_NOINDEX, beg, len);
        }

        TStringPiece AppendText(const TWtringBuf& buf) {
            if (Builder.NodeCount() == 0) {
                return TStringPiece(TStringPiece::SPS_TEXT);
            }
            ui32 beg = Builder.TextBuf().size();
            Builder.TextBuf().append(buf);
            ui32 len = Builder.TextBuf().size() - beg;
            TokenizedText.Update(beg, len);
            return TStringPiece(TStringPiece::SPS_TEXT, beg, len);
        }

        void PushToken(const TWideToken& token, TPosting pos) {
            if (Builder.NodeCount() == 0) {
                return;
            }
            if (token.SubTokens.size() > 1) {
                for (size_t i = 0; i < token.SubTokens.size(); i++) {
                    const auto& st = token.SubTokens[i];
                    TStringPiece tokenPiece = AppendText(TWtringBuf(token.Token + st.Pos, st.Len));
                    TTokenData::ETokenMode mode = (i == 0 ? TTokenData::TM_MULTI_START : TTokenData::TM_MULTI_BODY);
                    Builder.PushTokenData(TTokenData(CurrentParent, pos, tokenPiece, st.Type, mode));

                    if (i < token.SubTokens.size() - 1) {
                        const auto& next = token.SubTokens[i + 1];
                        TWtringBuf delim(token.Token + st.EndPos(), next.Pos - st.EndPos());
                        if (!delim.empty()) {
                            Builder.PushTokenData(TTokenData(CurrentParent, pos, AppendText(delim), TOKEN_MIXED, TTokenData::TM_MULTI_DELIM));
                        }
                    }
                }
            } else {
                Builder.PushTokenData(TTokenData(CurrentParent, pos, AppendText(TWtringBuf(token.Token, token.Leng)), token.SubTokens.begin()->Type, TTokenData::TM_SINGLE));
            }
        }

        bool OkBreak(ui16 bt) {
            return (bt & ST_SENTBRK) || (bt & ST_PARABRK);
        }

        void UpdateBreak(ui16& oldBT, const ui16 newBT) {
            oldBT |= static_cast<ui16>(newBT & (ST_SENTBRK | ST_PARABRK));
        }

        void PushBreak(TBreakType bt, bool findNonEmpty) {
            if (Builder.TokenCount() > 0 && OkBreak(bt)) {
                if (!findNonEmpty) {
                    TTokenData& last = Builder.TokenData(Builder.TokenCount() - 1);
                    UpdateBreak(last.BreakType, bt);
                    return;
                }
                for (int i = Builder.TokenCount() - 1; i >= 0; i--) {
                    TTokenData& token = Builder.TokenData(i);
                    if (token.TokenMode == TTokenData::TM_SPACES_EMPTY) {
                        continue;
                    } else {
                        UpdateBreak(token.BreakType, bt);
                        break;
                    }
                }
            }
        }

        void PushSpaces(TBreakType bt, const TWtringBuf& spaces, TPosting pos) {
            if (Builder.NodeCount() == 0) {
                return;
            }

            bool tokensEmpty = Builder.TokenCount() == 0;
            bool spacesOnly = IsSpace(spaces);
            if (tokensEmpty && spacesOnly) {
                return;
            }
            if (!spaces.empty()) {
                TUtf16String prepSpaces;
                if (tokensEmpty) {
                    prepSpaces = ProcessSpaces(spaces, true);
                } else {
                    if (spacesOnly) {
                        const TTokenData& last = Builder.TokenData(Builder.TokenCount() - 1);
                        if (last.TokenMode == TTokenData::TM_SPACES_EMPTY) {
                            TWtringBuf buf(Builder.TextBuf().begin() + last.TokenText.Start, last.TokenText.Len);
                            if (!buf.empty() && !IsSpace(buf.back())) {
                                prepSpaces = u" ";
                            }
                        } else {
                            prepSpaces = ProcessSpaces(spaces, false);
                        }
                    } else {
                        prepSpaces = ProcessSpaces(spaces, false);
                    }
                }
                if (!TrimSpaces) {
                    prepSpaces = spaces;
                }
                if (!prepSpaces.empty()) {
                    TTokenData::ETokenMode mode = IsSpace(prepSpaces) ? TTokenData::TM_SPACES_EMPTY : TTokenData::TM_SPACES;
                    Builder.PushTokenData(TTokenData(CurrentParent, pos, AppendText(prepSpaces), TOKEN_MIXED, mode));
                }
            }
            PushBreak(bt, TrimSpaces);
        }
    };

    class TTreeBuilderHandler: public ITreeBuilderHandler {
    private:
        THolder<TTreeBuilder> TreeBuilder;
        TTreeBuilderPtr DataBuilder;
        IPositionProvider* PositionProvider{nullptr};
        TStringBuf ParsedDocument;
        bool DecodeHtEntity{false};
        bool ProcessScriptText{false};

    public:
        TTreeBuilderHandler(TTreeBuilderPtr builder = TTreeBuilderPtr())
            : DataBuilder(builder)
        {
        }

        void SetPositionProvider(IPositionProvider* positionProvider) override {
            PositionProvider = positionProvider;
        }

        void SetParsedDocument(TStringBuf document) override {
            ParsedDocument = document;
        }

        void SetDecodeHtEntity(bool decode) override {
            DecodeHtEntity = decode;
        }

        void SetProcessScriptText(bool processScriptText) override {
            ProcessScriptText = processScriptText;
        }

        void OnTextStart(const IParsedDocProperties* prop) override {
            Y_ASSERT(nullptr != prop);
            if (nullptr == DataBuilder.Get()) {
                DataBuilder = CreateTreeBuilder();
            }
            TreeBuilder.Reset(new TTreeBuilder(*DataBuilder.Get(), prop->GetCharset(), DecodeHtEntity, ProcessScriptText));
            TreeBuilder->SetPositionProvider(PositionProvider);
            TreeBuilder->SetParsedDocument(ParsedDocument);
        }

        void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* /*ze*/, const TNumerStat& stat) override {
            switch (chunk.GetLexType()) {
                case HTLEX_START_TAG:
                    TreeBuilder->StartNode(chunk, stat);
                    break;
                case HTLEX_EMPTY_TAG:
                    TreeBuilder->StartNode(chunk, stat);
                    TreeBuilder->StopNode(chunk, stat);
                    break;
                case HTLEX_END_TAG:
                    TreeBuilder->StopNode(chunk, stat);
                    break;
                case HTLEX_TEXT:
                    TreeBuilder->PutText(chunk, stat);
                    break;
                default:
                    break;
            }
        }

        void OnTokenStart(const TWideToken& token, const TNumerStat& s) override {
            return; // temporarily unused method
            TPosting pos;
            SetPosting(pos, s.TokenPos.Break(), s.TokenPos.Word(), MID_RELEV);
            TreeBuilder->PushToken(token, pos);
        }

        void OnSpaces(TBreakType bt, const wchar16* t, unsigned length, const TNumerStat& s) override {
            return; // temporarily unused method
            TPosting pos;
            SetPosting(pos, s.TokenPos.Break(), s.TokenPos.Word(), MID_RELEV);
            TreeBuilder->PushSpaces(bt, TWtringBuf(t, length), pos);
        }

        TDomTreePtr GetTree() const override {
            return DataBuilder;
        }

        size_t GetMaxDepth() const override {
            return TreeBuilder->GetMaxDepth();
        }
    };

    TNumHandlerPtr TreeBuildingHandler() {
        return TNumHandlerPtr(new TTreeBuilderHandler());
    }

    // implementation for tests, see declaration in ut/testhandler.cpp
    TNumHandlerPtr TreeBuildingHandler(TTreeBuilderPtr builder) {
        return TNumHandlerPtr(new TTreeBuilderHandler(builder));
    }

}
