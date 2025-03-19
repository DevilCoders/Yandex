#pragma once

#include <kernel/lemmer/core/disamb_options.h>
#include <library/cpp/tokenclassifiers/token_types.h>
#include <util/memory/blob.h>
#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>
#include <kernel/qtree/request/nodebase.h>
#include <kernel/qtree/request/reqlenlimits.h>

#include "markup/markupiterator.h"
#include "markup/synonym.h"
#include "nodesequence.h"
#include "proxim.h"
#include "wordnode.h"

#include "richnode_fwd.h"

#include "object_identifier.h"

struct TCharSpan;
class TLanguageContext;

class TReqAttrList;
struct TRequestNode;

class TPure;

namespace NRichTreeProtocol {
    class TRichRequestNode;
};

namespace NSearchQuery {
    class TMarkup;
}

namespace NRichTreeBuilder {
    class TTreeCreator;
}

namespace NMorphoLangDiscriminator {
    class TContext;
}

enum EGeoType  {
    GEO_UNK,
    GEO_ADM,
    GEO_CITY,
    GEO_CONTINENT,
    GEO_COUNTRY,
    GEO_DISTRICT,
    GEO_VILL
};

enum EHiliteType {
    HILITE_NONE = 0, // don't hilite
    HILITE_SELF, // hilite if this word has matched
    HILITE_LEFT, // match this word + left one
    HILITE_RIGHT,
    HILITE_LEFT_OR_RIGHT,
    HILITE_LEFT_AND_RIGHT,
};

//SNIPPETS-1444
enum ESnippetType {
    SNIPPET_SELF = 0, // include if this word has matched
    SNIPPET_NONE = 1, // don't include
};

enum EGeoRuleType {
    GEO_RULE_NONE = 0,
    GEO_RULE_ADDR = 1,
    GEO_RULE_CITY = 2,
    GEO_RULE_BOTH = GEO_RULE_ADDR | GEO_RULE_CITY,
};

void Serialize(const TRichRequestNode& node, NRichTreeProtocol::TRichRequestNode& message, bool humanReadable);
TRichNodePtr Deserialize(const NRichTreeProtocol::TRichRequestNode& message, EQtreeDeserializeMode mode = QTREE_DEFAULT_DESERIALIZE);

class TRichRequestNode : public TRequestNodeBase
                       , public TAtomicRefCount<TRichRequestNode>
                       , public TNonCopyable
{
friend class NRichTreeBuilder::TTreeCreator;
friend class TSynonym;
friend class NSearchQuery::TRequest;
friend void Serialize(const TRichRequestNode& node, NRichTreeProtocol::TRichRequestNode& message, bool humanReadable);
friend TRichNodePtr Deserialize(const NRichTreeProtocol::TRichRequestNode& message, EQtreeDeserializeMode mode);

private:
    typedef TObjectIdentifier<TRichRequestNode> TIdObject;

public:
    typedef TVector<TRichNodePtr> TNodesVector;
    typedef TIdObject::TId TId;
    class TStatInfo {
    private:
        ui64 HitsCount = 0;
        ui64 GlobalDocsCount = 0;
    public:
        void SetHitsCount(ui64 hitsCount) {
            HitsCount = hitsCount;
        }

        ui64 GetHitsCount() const {
            return HitsCount;
        }

        void SetGlobalDocsCount(ui64 globalDocsCount) {
            GlobalDocsCount = globalDocsCount;
        }

        ui64 GetGlobalDocsCount() const {
            return GlobalDocsCount;
        }

    };
private:
    // ATTENTION: do not forget about Swap() and Copy() methods when adding new data members
    TIdObject Id;

    EGeoType GeoType;
    ui32 AttrIntervalRestriction;
    EHiliteType HiliteType;
    ESnippetType SnippetType;

    TFormType FormType; // WordInfo overrides it

    EGeoRuleType Geo;
    mutable TAtomicSharedPtr<TStatInfo> StatInfo = nullptr;
private:
    TRichRequestNode(const TRequestNodeBase& src, TNodeNecessity contextNecessity, TFormType formType);
    TRichRequestNode(TFormType formType = fGeneral, TWordJoin = WORDJOIN_UNDEF, TWordJoin = WORDJOIN_UNDEF);
public:
    static const TProximity DisjunctionProxy;

    TNodeSequence Children;     // Markup is now owned by Children
    TVector<TRichNodePtr> MiscOps;

    THolder<TWordNode> WordInfo;

public:
    EGeoType GetGeoType() const;
    void SetGeoType(EGeoType type);

    ui64 GetExpectedDocumentsCount() const;

    TStatInfo& MutableStatInfo() const {
        Y_VERIFY(!!StatInfo);
        return *StatInfo;
    }

    template <class TOp>
    void Scan(TOp op) {
        op(*this);
        for (auto&& i : Children) {
            i->Scan<TOp>(op);
        }
    }

    bool HasGeo() const;
    bool HasGeoRuleType(EGeoRuleType type) const;
    bool HasGeoAddr() const;
    bool HasGeoCity() const;
    EGeoRuleType GetGeoRuleType() const;
    void SetGeoRuleType(EGeoRuleType type);
    void SetGeoAddr();
    void SetGeoCity();

    TId GetId() const;

    EHiliteType GetHiliteType() const;
    void SetHiliteType(EHiliteType type);

    ESnippetType GetSnippetType() const;
    void SetSnippetType(ESnippetType type);

    //Checking for node equality, differences in extension type are ignored
    bool Compare(const TRichRequestNode* node) const;
    static bool CompareVectors(const TNodesVector& left, const TNodesVector& right);

    TRichNodePtr Copy() const;
    void Swap(TRichRequestNode& node);

    bool IsLeaf() const {
        return Children.empty();
    }

    bool IsRestriction() const;
    bool IsOrdinaryWord() const;
    bool IsExactWord() const;
    bool IsAttributeInterval() const;
    bool IsPhrase(bool checkMiscOps = true) const;
    bool IsBinaryOp() const;
    bool IsAndNotOp() const;
    bool IsWeakOr() const;
    bool IsEqualWord(const TWtringBuf& word) const;
    bool IsNumber() const;
    bool IsInteger() const;
    bool IsFloat() const;
    bool IsMark() const;
    bool IsStopWord() const;
    bool IsExtension() const; //return false
    bool IsAuxiliaryWord() const;
    bool ChildrenJustAuxillary() const;

    // both functions are unsafe on nodes with diacritics, see https://st.yandex-team.ru/REQWIZARD-715
    double GetNumber() const;
    int GetInteger() const;

    TFormType GetFormType() const;
    void SetFormType(TFormType formType);

    void SetParens(bool parens);

    size_t CalcAttributeIntervalsCount() const;

    void SetAttrIntervalRestriction(ui32 r);
    ui32 GetAttrIntervalRestriction() const;

    size_t GetChildPos(const TRichNodePtr& child) const;
    size_t GetChildPos(const TRichRequestNode& child) const;
    NSearchQuery::TRange GetChildRange(const TRichNodePtr& child, bool allowSyn = true) const;
    NSearchQuery::TRange GetChildRange(const TRichRequestNode& child, bool allowSyn = true) const;
    void RemoveChildren(size_t frompos, size_t topos);

    void ReplaceChild(size_t pos, const TRichNodePtr& node);
    void ReplaceChildren(size_t pos, size_t count, const TRichNodePtr& node);

    void InsertChild(size_t pos, TRichNodePtr node, const TProximity& distance /*distance to previous node*/);

    void AddZoneFilter(const TString& zoneName);
    void AddRefineFilter(const TRichNodePtr& restriction);
    void AddRefineFactorFilter(const TRichNodePtr& restriction, const TStringBuf& factorName, float factorVal = 1.f);
    void AddAndNotFilter(const TRichNodePtr& restriction);
    void AddRestrDocFilter(const TRichNodePtr& restriction);
    void AddRestrictByPosFilter(const TRichNodePtr& restriction);

    static bool HasExtension(const TRichNodePtr& node, TThesaurusExtensionId type);

    void SetStopWordHiliteMode();

    void MakeNoBreakLevelDistance(bool convert);
    bool HasQueryLanguage() const;
    void MakeSoftDistances(bool keepUserDistance = false);

    bool HasAnyLemma(const THashSet<TUtf16String>& lemmas) const;
    void CollectLemmas(TVector<TUtf16String>& lemmas) const;
    void CollectLemmas(THashSet<TUtf16String>& lemmas) const;
    void CollectForms(TVector<TUtf16String>& forms) const;

    void UpdateKeyPrefix(const TVector<ui64>& value);



    const NSearchQuery::TMarkup& Markup() const {
        return Children.Markup();
    }

    NSearchQuery::TMarkup& MutableMarkup() {
        return Children.MutableMarkup();
    }

    void AddMarkup(size_t beg, size_t end, const NSearchQuery::TMarkupDataPtr& data); // @end is the last node of markup range, same as TRange::End

    void AddMarkup(size_t childIndex, const NSearchQuery::TMarkupDataPtr& data) {               // single-node markup
        AddMarkup(childIndex, childIndex, data);
    }

    void AddMarkup(const TRichRequestNode& child, const NSearchQuery::TMarkupDataPtr& data) {   // single-node markup
        AddMarkup(GetChildPos(child), data);
    }

    // markup over all children
    void AddMarkup(const NSearchQuery::TMarkupDataPtr& data) {
        AddMarkup(0, Children.empty() ? 0 : Children.size() - 1, data);
    }

    // Find position (parent and children index range) for attaching markup
    // to a range of nodes between @first and @last inclusively.
    // @first and @last should be among sub-nodes of @this (on any depth), @first < @last in DFS order.
    // Return NULL when no proper non-ambiguous position exists.
    TRichRequestNode* FindMarkupPosition(const TRichRequestNode& first, const TRichRequestNode& last, size_t& beg, size_t& end, bool allowSyn = false);

    // Search @beg and @end nodes in all children recursively, find proper parent and markup position and add markup there.
    // Return false if cannot find proper position (see FindMarkupPosition)
    bool AddMarkupDeep(const TRichRequestNode& beg, const TRichRequestNode& end, const NSearchQuery::TMarkupDataPtr& data, bool allowSyn = false);


    bool AddSynonym(const TRichRequestNode& beg, const TRichRequestNode& end, TAutoPtr<TSynonym> synonym, bool allowSyn = false);
    bool AddSynonym(const TRichRequestNode& node, TAutoPtr<TSynonym> synonym, bool allowSyn = false) {
        return AddSynonym(node, node, synonym, allowSyn);
    }



    template<class TConcreteMarkupData>
    NSearchQuery::TPosMarkupIterator<TConcreteMarkupData> GetChildMarkupIterator(size_t pos) const {
        return NSearchQuery::TPosMarkupIterator<TConcreteMarkupData>(Markup(), pos);
    }
    template<class TConcreteMarkupData>
    NSearchQuery::TPosMarkupIterator<TConcreteMarkupData> GetChildMarkupIterator(const TRichRequestNode& child) const {
        return GetChildMarkupIterator<TConcreteMarkupData>(GetChildPos(child));
    }
    template<class TConcreteMarkupData>
    bool IsChildCoveredByMarkup(size_t childPos) const {
        return !GetChildMarkupIterator<TConcreteMarkupData>(childPos).AtEnd();
    }
    template<class TConcreteMarkupData>
    bool IsChildCoveredByMarkup(const TRichRequestNode& child) const {
        size_t pos = Children.FindPosition(&child);
        if (pos == size_t(-1))
            return false;
        return IsChildCoveredByMarkup<TConcreteMarkupData>(pos);
    }
    template<class TConcreteMarkupData, class TChecker>
    bool IsChildCoveredByMarkup(size_t childPos, const TChecker& checker) const {
        return !NSearchQuery::TGeneralCheckMarkupIterator<NSearchQuery::TPosMarkupIterator<TConcreteMarkupData>, TChecker> (GetChildMarkupIterator<TConcreteMarkupData>(childPos), checker).AtEnd();
    }
    template<class TConcreteMarkupData, class TChecker>
    bool IsChildCoveredByMarkup(const TRichRequestNode& child, const TChecker& checker) const {
        size_t pos = Children.FindPosition(&child);
        if (pos == size_t(-1))
            return false;
        return IsChildCoveredByMarkup<TConcreteMarkupData, TChecker>(pos, checker);
    }


    void VerifyConsistency() const;
    TUtf16String Print(EPrintRichRequestOptions format = PRRO_Default) const; // same as PrintRichRequest()
    TString DebugString() const; // simplified tree structure representation for debugging mainly
};

typedef TRichRequestNode::TId TRichNodeId;

namespace NSearchQuery {

class TRequest : public TAtomicRefCount<TRequest>
               , public TNonCopyable
{
public:
    TRequest()
        : Softness(0)
        , FoundTokenTypes(0x0)
    {
    }

    // Compact serialization for binary transmission of the tree
    void Serialize(TBinaryRichTree& buffer) const;
    void Deserialize(const TBinaryRichTree& buffer, EQtreeDeserializeMode mode = QTREE_DEFAULT_DESERIALIZE);
    // Readable serialization (for debugging purposes ONLY)
    void SerializeToClearText(TString& data) const;
    TString ToJson() const;

    void DeserializeFromClearText(const TString& data);

    bool Compare(const TRequest* tree) const; //differences in extension type are ignored
    TRichTreePtr Copy() const;
public:
    ui32 Softness;
    TRichNodePtr Root;
    NSearchRequest::TOperatorsFound FoundReqLang;
    NTokenClassification::TTokenTypes FoundTokenTypes;
};

}

//-----------------------------------------------------

class TCreateTreeOptions {
public:
    explicit TCreateTreeOptions(const TLanguageContext& lang = TLanguageContext(), ui32 reqflags = 0);
    explicit TCreateTreeOptions(const TLangMask& mask);
    TCreateTreeOptions(const TLanguageContext& lang, const TReqAttrList* attrList, ui32 reqflags = 0);
    TCreateTreeOptions(const TLanguageContext& lang, const char* attrList, ui32 reqflags = 0);

    TLengthLimits Limits;
    TLanguageContext Lang;
    ui32 Reqflags;
    const TReqAttrList* AttrListTReq;
    const char* AttrListChar;
    TDisambiguationOptions DisambOptions;
    bool MakeSynonymsForMultitokens;
    bool GenerateForms;
    bool AddDefaultLemmas = true;
    // Do not lemmatize words in tree
    bool NoLemmerNodes = false;

    size_t TokenizerVersion = 1;

    const TLangMask& GetPrefLangMask() const {
        return DisambOptions.PreferredLangMask;
    }
    void SetPrefLangMask(const TLangMask& mask) {
        DisambOptions.PreferredLangMask = mask;
    }

    TString DebugString() const {
        auto bool2str = [](bool b) { return b ? "yes" : "no"; };
        TString s = TString("Lang.IsStopWord('як'): ") + bool2str(Lang.IsStopWord("як"))
                 + "\nReqFlags: " + ToString(Reqflags)
                 + "\nAttrListChar: " + ToString(AttrListChar)
                 + "\nMakeSynonymsForMultitokens: " + bool2str(MakeSynonymsForMultitokens)
                 + "\nGenerateForms: " + bool2str(GenerateForms)
                 + "\nAddDefaultLemmas: " + bool2str(AddDefaultLemmas)
                 + "\nNoLemmerNodes: " + bool2str(NoLemmerNodes);
        return s;
    }
};

struct TCreateRichNodeResults {
    TRichNodePtr Root;
    TLangMask RequestLanguages; // depends on initial value of UsrLanguages
    TLangMask UsrLanguages; // its value is also input
    ::NTokenClassification::TTokenTypes FoundTokenTypes;

    void SaveLanguages(
        TLangMask* requestLanguages,
        TLangMask* usrLanguages)
    {
        if (requestLanguages) {
            *requestLanguages = RequestLanguages;
        }
        if (usrLanguages) {
            *usrLanguages = UsrLanguages;
        }
    }
};

struct TCreateRichTreeResults
    : public TCreateRichNodeResults
{
    TRichTreePtr Tree;
};

//------------------------------------------

void CreateRichNode(const TUtf16String& query, const TCreateTreeOptions& treeOpt, TCreateRichNodeResults& results);
void CreateRichTree(const TUtf16String& query, const TCreateTreeOptions& treeOpt, TCreateRichTreeResults& results);

inline TRichNodePtr CreateRichNode(const TUtf16String& query, const TCreateTreeOptions& treeOpt) {
    TCreateRichNodeResults results;
    CreateRichNode(query, treeOpt, results);
    return results.Root;
}

inline TRichNodePtr CreateRichNode(const TUtf16String& query, const TCreateTreeOptions& treeOpt, TLangMask* requestLanguages, TLangMask* usrLanguages = nullptr) {
    TCreateRichNodeResults results;
    if (usrLanguages) {
        results.UsrLanguages = *usrLanguages;
    }
    CreateRichNode(query, treeOpt, results);
    results.SaveLanguages(requestLanguages, usrLanguages);
    return results.Root;
}

inline TRichTreePtr CreateRichTree(const TUtf16String& query, const TCreateTreeOptions& treeOpt) {
    TCreateRichTreeResults results;
    CreateRichTree(query, treeOpt, results);
    return results.Tree;
}

inline TRichTreePtr CreateRichTree(const TUtf16String& query, const TCreateTreeOptions& treeOpt, TLangMask* requestLanguages, TLangMask* usrLanguages = nullptr) {
    TCreateRichTreeResults results;
    if (usrLanguages) {
        results.UsrLanguages = *usrLanguages;
    }
    CreateRichTree(query, treeOpt, results);
    results.SaveLanguages(requestLanguages, usrLanguages);
    return results.Tree;
}

// CreateRichNode(Tree) will throw exception on some queries (if they don't contain any alpha-numeric characters, for example).
// These versions will return NULL in such cases (so do not forget to check!)
TRichNodePtr TryCreateRichNode(const TUtf16String& query, const TCreateTreeOptions& treeOpt);
TRichTreePtr TryCreateRichTree(const TUtf16String& query, const TCreateTreeOptions& treeOpt);

void CreateRichNodeFromBinaryTree(const TRequestNode* node, const TCreateTreeOptions& options, TCreateRichNodeResults& results);
void CreateRichTreeFromBinaryTree(const TRequestNode* node, const TCreateTreeOptions& options, NSearchRequest::TOperatorsFound operatorsFound, TCreateRichTreeResults& results);

inline TRichNodePtr CreateRichNodeFromBinaryTree(const TRequestNode* node, const TCreateTreeOptions& options) {
    TCreateRichNodeResults results;
    CreateRichNodeFromBinaryTree(node, options, results);
    return results.Root;
}

inline TRichNodePtr CreateRichNodeFromBinaryTree(const TRequestNode* node, const TCreateTreeOptions& options, TLangMask* requestLanguages, TLangMask* usrLanguages = nullptr) {
    TCreateRichNodeResults results;
    if (usrLanguages) {
        results.UsrLanguages = *usrLanguages;
    }
    CreateRichNodeFromBinaryTree(node, options, results);
    results.SaveLanguages(requestLanguages, usrLanguages);
    return results.Root;
}

inline TRichTreePtr CreateRichTreeFromBinaryTree(const TRequestNode* node, const TCreateTreeOptions& options, NSearchRequest::TOperatorsFound operatorsFound = {}) {
    TCreateRichTreeResults results;
    CreateRichTreeFromBinaryTree(node, options, operatorsFound, results);
    return results.Tree;
}

inline TRichTreePtr CreateRichTreeFromBinaryTree(const TRequestNode* node, const TCreateTreeOptions& options, TLangMask* requestLanguages, TLangMask* usrLanguages = nullptr,
                                          NSearchRequest::TOperatorsFound operatorsFound = {})
{
    TCreateRichTreeResults results;
    if (usrLanguages) {
        results.UsrLanguages = *usrLanguages;
    }
    CreateRichTreeFromBinaryTree(node, options, operatorsFound, results);
    results.SaveLanguages(requestLanguages, usrLanguages);
    return results.Tree;
}

TRichNodePtr CreateEmptyRichNode();
TRichTreePtr CreateEmptyRichTree();

TLangMask DisambiguateLanguagesTree(TRichRequestNode* root, const TDisambiguationOptions& options, const TLangMask& addMainLang);

void UpdateRichTree(TRichNodePtr& tree, bool sortMarkup = false, bool splitMultitokens = false);
/// Heuristically undo UpdateRichTree. This function is not precise (REQWIZARD-1173)
void UnUpdateRichTree(TRichNodePtr& tree);
void CollectSubTree(TRichNodePtr& tree, bool ignoreParens = false);

void SimpleUnMarkRichTree(TRichNodePtr& node);

bool FixPlainNumbers(TRichNodePtr& node);

TRichTreePtr DeserializeRichTree(const TBinaryRichTree& buffer, EQtreeDeserializeMode mode = QTREE_DEFAULT_DESERIALIZE);

TString EncodeRichTreeBase64(const TBinaryRichTree& tree, bool url = false);
TBinaryRichTree DecodeRichTreeBase64(const TStringBuf& str);

TString CreatePackedTree(const TUtf16String& query, const TCreateTreeOptions& treeOpt, const TPure* pure);

bool DeleteMarkerNode(TRichNodePtr& parent, const TRichRequestNode* deleteTrg);

void UpdateRedundantLemmas(TRichNodePtr& node);

inline bool IsWordInfoNode(const TRichRequestNode& n) {
    return IsWordOrMultitoken(n) && !!n.WordInfo;
}

ui32 CalcWordCount(const TRichRequestNode& node, bool nonStop = false);


class TCorruptedTreeException: public yexception {
};

inline bool IsConsistent(const TRichRequestNode& node) {
    try {
        node.VerifyConsistency();
    } catch (const TCorruptedTreeException& /*e*/) {
        return false;
    }
    return true;
}

#define VERIFY_RICHTREE(tree, message)                             \
    try {                                                             \
        (tree).VerifyConsistency();                                   \
    } catch (const TCorruptedTreeException& e) {                      \
        ythrow TCorruptedTreeException() << (message) << e.what();    \
    }
