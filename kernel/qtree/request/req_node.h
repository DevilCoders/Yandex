#pragma once

#include <util/system/defaults.h>

#include <util/generic/ptr.h>
#include <util/memory/alloc.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/intrlist.h>
#include <util/generic/noncopyable.h>
#include <util/charset/wide.h>

#include <library/cpp/token/nlptypes.h>
#include <library/cpp/token/token_structure.h>

#include "nodebase.h"


struct TLangPrefix;
struct TLangSuffix;
class TLangEntry;
class TReqTokenizer;
class tRequest;

//! if softness is 0 it is not printed
const ui32 SOFTNESS_MIN = 0;
const ui32 SOFTNESS_MAX = 100;

//! @attention GC always contains all nodes, but the root is deleted by a smart pointer;
//!            nodes must not be deleted by the 'delete' operator (a smart pointer) at all excepting the root;
//!            you must not call to TRequestNode::Unlink() directly, use TGc::Release();
struct TRequestNode: public TRequestNodeBase, public TIntrusiveListItem<TRequestNode> {
    class TGc: public TSimpleRefCount<TGc>, private TNonCopyable {
        public:
            TGc()
                : Count(0)
            {
            }

            inline ~TGc() {
                while (!mGC.Empty()) {
                    delete mGC.PopBack();
                    --Count;
                }
                Y_ASSERT(Count == 0);
            }

            inline TRequestNode* Add(TRequestNode* node) noexcept {
                if (node) {
                    mGC.PushBack(node);
                    ++Count;
                }

                return node;
            }

            void Release(TRequestNode* node) noexcept {
                if (node && !node->Empty()) {
                    Y_ASSERT(node->GC.Get() && node->GC->RefCount() == 1); // only root must be released from ~TRequestNode()
                    node->Unlink();
                    Y_ASSERT(Count > 0);
                    --Count;
                }
            }

            //! creates an attribute node (or incomplete zone node) based on the information prepared by the scanner
            TRequestNode* CreateAttrTypeNode(const TLangEntry& le) {
                return Add(new TRequestNode(le));
            }

            TRequestNode* CreateWordNode(const TLangEntry& le,
                                         const TWideToken& tok,
                                         NTokenClassification::TTokenTypes tokenTypes = 0)
            {
                return Add(new TRequestNode(le, tok, tokenTypes));
            }

            //! creates binary operator node with left and right operands
            //! @param left     the node that represents the left operand
            //! @param right    the node that represents the left operand
            //! @param lp       the prefix of the new operator node
            //! @param ls       the suffix of the new operator node
            TRequestNode* CreateBinaryOper(TRequestNode* left, TRequestNode* right, const TLangPrefix& lp, const TLangSuffix& ls, TPhraseType ft) {
                return Add(new TRequestNode(left, right, lp, ls, ft));
            }

            //! creates a clone of this node
            //! @note the @c Name and @c Span members must be changed after call to this function
            TRequestNode* Clone(const TRequestNode& node) {
                return Add(new TRequestNode(node, *this));
            }

            size_t GetNodesCount() const {
                return Count;
            }

        private:
            TIntrusiveList<TRequestNode> mGC;
            size_t Count;
    };

    typedef TGc TFactory;
    typedef TIntrusivePtr<TFactory> TFactoryPtr;
    typedef TIntrusivePtr<TGc> TGcRef;
private:
    TRequestNode(const TRequestNode& node, TFactory& factory);
    // leaf node (attribute or word) :
    explicit TRequestNode(const TLangEntry& le);

    //! word node constructed
    //! @param tok      length can include prefix and suffix
    //! @note FormType, Necessity, Idf and Text are used from TLangEntry
    TRequestNode(const TLangEntry& le,
                 const TWideToken& tok,
                 NTokenClassification::TTokenTypes = 0);

    //! binary operator constructed
    TRequestNode(TRequestNode* left, TRequestNode* right, const TLangPrefix& lp, const TLangSuffix& ls, TPhraseType ft);

private:
    //! @note softness is not copied and not compared, it can only be set by @c SetSoftness
    //!       because only the root node can have non-zero softness
    ui32 Softness;
    // Actually prefixes in SubTokens are not used, they are removed in two stages: in StorePunctuation() and by TSubWord when richtree is built.
    // This prefix is not serialized and used only for creation of "synonym" for word with prefix in richtree.
    TUtf16String Prefix;

public:
    TRequestNode* Left;
    TRequestNode* Right;
    TFormType FormType;
    TCharSpan Span;      // span of the node in original request, length includes suffix (no prefix) TODO:what parts for what node types
    TCharSpan FullSpan;  // full span of the node in the original request.
    TGcRef GC;

public:
    ~TRequestNode();

    ui32 GetSoftness() const {
        return Softness;
    }

    void SetSoftness(ui32 value) {
        Y_ASSERT(value <= SOFTNESS_MAX);
        Softness = value;
    }

    //! @note multotoken with number of subtokens greater than 1 is not considered as a leaf node because in rich tree
    //!       it will be transformed into subtree
    int IsLeaf() const {
        return !Left && !Right && (SubTokens.size() <= 1); // title:((a-b)) is broken without checking size of multitoken
    }

    //! replaces @c PHRASE_PHRASE with the appropriate Level: document or sentence
    //! @attention this method is called by @c tRequest::Analyze() in Yandex.Server and Yandex.Market
    void TreatPhrase(const TOpInfo& phraseOperInfo, TPhraseType defaultType);

    void SetOpInfo(const TOpInfo& opInfo) {
        Y_ASSERT(!IsAttributeOrZone(*this));
        OpInfo = opInfo;
    }

    void UpdateKeyPrefix(const TVector<ui64>& keyPrefix) {
        if (Left)
            Left->UpdateKeyPrefix(keyPrefix);
        if (Right)
            Right->UpdateKeyPrefix(keyPrefix);
        SetKeyPrefix(keyPrefix);
    }

    void SetNGrammInfo(ui8 value, ui8 ngrZoneSoftness) {
        TRequestNodeBase::SetNGrammBase(value, ngrZoneSoftness);
        if (Left)
            Left->SetNGrammInfo(value, ngrZoneSoftness);
        if (Right)
            Right->SetNGrammInfo(value, ngrZoneSoftness);
    }

    void UpdateNGrammInfo(const TReqAttrList& attrList, const TMap<TString, ui32>& softnessData) {
        if (OpInfo.Op == oZone) {
            auto* data = attrList.GetAttrData(GetTextName());
            if (data->NGrammBase) {
                ui32 ngrZoneSoftness = 50;
                TString key = WideToUTF8(GetTextName());
                if (const auto* const p = softnessData.FindPtr(key)) {
                    ngrZoneSoftness = *p;
                }
                if (Left)
                    Left->SetNGrammInfo(data->NGrammBase, ngrZoneSoftness);
                if (Right)
                    Right->SetNGrammInfo(data->NGrammBase, ngrZoneSoftness);
                return;
            }
        } else {
            if (Left)
                Left->UpdateNGrammInfo(attrList, softnessData);
            if (Right)
                Right->UpdateNGrammInfo(attrList, softnessData);
        }
    }

    void ConvertNameToText() {
        const TUtf16String& name = GetTextName();
        SetLeafWord(name, TCharSpan(0, name.size()));
    }

    void RemovePrefix() {
        TRequestNodeBase::RemovePrefix(Span);
    }

    const TUtf16String& GetPrefix() const {
        return Prefix;
    }
};

TRequestNode* ConvertAZNode(TRequestNode* pNode, TRequestNode* right);

extern const char ATTR_PREVREQ_STR[];
extern const char ATTR_SOFTNESS_STR[];
extern const char ATTR_INPOS_STR[];
extern const char ATTR_REFINE_FACTOR_STR[];

//! creates zone or attribute node depended on the name
//! @param tokenizer    request tokenizer containing all information prepared by the scanner
//! @param tokend       the end of the token (name:value)
//! @param lval         pointer to the YYSTYPE bison union
//! @param rType        output bison type: RESTRBYPOS or ATTR_VALUE
int CreateZoneOrAttr(TReqTokenizer& tokenizer, const wchar16* tokend, void* lval);

//! creates a zone node
//! @param tokenizer    request tokenizer containing all information prepared by the scanner
//! @param tokend       the end of the token (name:value)
int CreateZoneNode(const TReqTokenizer& tokenizer, const wchar16* tokend, void* lval);

//! creates a node with no child nodes that will be passed to @c ConvertAZNode as a zone node
TRequestNode* CreateZoneNode(const TReqTokenizer& tokenizer, const TLangEntry& e);

//! splits text "name:value" into attribute name and value
//! @param tokenizer    request tokenizer containing all information prepared by the scanner
//! @param tokend       the end of the token (name:value)
//! @param quoted       @c true if value is quoted or in parentheses
int CreateAttrNode(const TReqTokenizer& tokenizer, const wchar16* tokend, void* lval, bool quoted);

//! returns name that is extracted from the information prepared by the scanner
//! @param entry    information prepared by the scanner
TUtf16String GetNameByEntry(const TLangEntry& entry);

//! returns length of a compare operator
//! @param oper     an operator that length is returned for
inline size_t GetCmpOperLen(TCompareOper oper)
{
    return (oper == cmpEQ ? 0 : (oper == cmpLT || oper == cmpGT ? 1 : 2));
}

TRequestNode* ConvertZoneExprToPhrase(TRequestNode* zone, TRequestNode* expr, const TReqTokenizer& tokenizer);

inline void SetPrefix(TRequestNode& n, TFormType ft, TNodeNecessity nec)
{
    if (n.FormType == fGeneral)
        n.FormType = ft;
    if (n.Necessity == nDEFAULT)
        n.Necessity = nec;
}

inline void SetSuffix(TRequestNode& n, int rf) {
    if (n.ReverseFreq == -1)
        n.ReverseFreq = rf;
}
