#pragma once

#include "decl.h"
#include "domtree.h"

#include <library/cpp/html/spec/tags.h>
#include <library/cpp/html/face/parstypes.h>
#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

#include <util/stream/str.h>

namespace NDomTree {
    struct TStringPiece {
        enum ESrc {
            SPS_UNK,
            SPS_ATTR,
            SPS_TEXT,
            SPS_NOINDEX
        };
        ESrc Src;
        ui32 Start;
        ui32 Len;

        TStringPiece(ESrc src, ui32 start = 0, ui32 len = 0)
            : Src(src)
            , Start(start)
            , Len(len)
        {
        }

        TString Dmp() const {
            TStringStream str;
            str << "Piece{src: ";
            switch (Src) {
                case SPS_UNK:
                    str << "unk";
                    break;
                case SPS_ATTR:
                    str << "attr";
                    break;
                case SPS_TEXT:
                    str << "text";
                    break;
                case SPS_NOINDEX:
                    str << "noind";
                    break;
            }
            str << ", start: " << Start << ", len: " << Len << "}";
            return str.Str();
        }
    };
    struct TAttrData {
        const ui32 NodeId;
        const TStringPiece Name;
        const TStringPiece Value;
        bool Boolean;
        ui64 Line = 0;
        ui64 Column = 0;

        TAttrData(
            ui32 node, const TStringPiece& name, const TStringPiece& val,
            bool boolean, ui64 line = 0, ui64 column = 0
        )
            : NodeId(node)
            , Name(name)
            , Value(val)
            , Boolean(boolean)
            , Line(line)
            , Column(column)
        {
        }
    };

    struct TTokenData {
        enum ETokenMode {
            TM_SPACES,
            TM_SPACES_EMPTY,
            TM_SINGLE,
            TM_MULTI_START,
            TM_MULTI_DELIM,
            TM_MULTI_BODY,
            TM_UNTOK
        };
        ui32 Parent;
        TPosting Pos;
        TStringPiece TokenText;
        ui16 TokenType; // ETokenType
        ETokenMode TokenMode;
        ui16 BreakType; // TBreakType

        TTokenData(ui32 parent, TPosting pos, const TStringPiece& text, ui16 tokenType, ETokenMode mode)
            : Parent(parent)
            , Pos(pos)
            , TokenText(text)
            , TokenType(tokenType)
            , TokenMode(mode)
            , BreakType(0) // ST_NOBRK
        {
        }
    };

    struct TNodeData {
        const ui32 Id;
        const ui32 Parent;
        const EDomNodeType Type;
        const HT_TAG HtmlTag = HT_any;
        const TEXT_WEIGHT Weight;
        TStringPiece TagName = TStringPiece(TStringPiece::SPS_UNK);
        TStringPiece Text = TStringPiece(TStringPiece::SPS_UNK);
        TPosting Beg = 0;
        TPosting End = 0;
        ui32 ChildBeg = 0;
        ui32 ChildEnd = 0;
        ui32 ChildCnt = 0;
        ui32 NextSibling = 0;
        ui32 PrevSibling = 0;
        ui32 AttrBeg = 0;
        ui32 AttrCnt = 0;
        ui32 FirstToken = 0;
        ui32 LastToken = 0;
        ui64 Line = 0;
        ui64 Column = 0;

        TNodeData(ui32 id, ui32 parent, TEXT_WEIGHT weight, HT_TAG tag, const TStringPiece& tagName)
            : Id(id)
            , Parent(parent)
            , Type(EDomNodeType::NT_Element)
            , HtmlTag(tag)
            , Weight(weight)
            , TagName(tagName)
        {
        }

        TNodeData(ui32 id, ui32 parent, TEXT_WEIGHT weight, const TStringPiece& str)
            : Id(id)
            , Parent(parent)
            , Type(EDomNodeType::NT_Text)
            , Weight(weight)
            , Text(str)
        {
        }
    };

    struct TTreeData {
        TUtf16String TextBuf;
        TUtf16String NoindexBuf;
        TString AttrBuf;
    };

    struct ITreeDataModifier: public IDomTree {
        virtual ui32 AttrCount() const = 0;
        virtual ui32 TokenCount() const = 0;
        virtual TNodeData& NodeData(ui32 id) = 0;
        virtual TAttrData& AttrData(ui32 id) = 0;
        virtual TTokenData& TokenData(ui32 id) = 0;
        virtual TNodeData& PushNodeData(const TNodeData& node) = 0;
        virtual void PushAttrData(const TAttrData& attr) = 0;
        virtual void PushTokenData(const TTokenData& token) = 0;
        virtual TString& AttrBuf() = 0;
        virtual TUtf16String& TextBuf() = 0;
        virtual TUtf16String& NoindexBuf() = 0;
        ~ITreeDataModifier() override = default;
        ;
    };

    using TTokenDataIterator = TVector<TTokenData>::const_iterator;
    using TTokenDataReverseIterator = TVector<TTokenData>::const_reverse_iterator;

    struct ITreeDataAccessor {
        virtual const IDomNode* GetNode(ui32 id) const = 0;
        virtual const IDomAttr* GetAttr(ui32 id) const = 0;
        virtual TStringBuf CText(const TStringPiece& p) const = 0;
        virtual TWtringBuf WText(const TStringPiece& p) const = 0;
        virtual ui32 TokenCount() const = 0;
        virtual const TTokenData& TokenData(ui32 id) const = 0;
        virtual TTokenDataIterator TokenDataBegin() const = 0;
        virtual TTokenDataIterator TokenDataEnd() const = 0;
        virtual ~ITreeDataAccessor() = default;
        ;
    };

    using TTreeBuilderPtr = TSimpleSharedPtr<ITreeDataModifier>;
    TTreeBuilderPtr CreateTreeBuilder();

}
