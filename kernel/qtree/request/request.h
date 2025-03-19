#pragma once

#include <new>

#include <util/system/defaults.h>
#include <util/generic/ptr.h>
#include <util/generic/noncopyable.h>
#include <util/string/vector.h>
#include <kernel/search_daemon_iface/reqtypes.h>
#include <kernel/reqerror/reqerror.h>
#include "reqattrlist.h"
#include <library/cpp/tokenclassifiers/token_types.h>

#include "fixreq.h"
#include "nodebase.h"
#include "req_node.h"

// uncomment this macro to debug the parsers
//#define TRACE_RULE_ORDER

class TQuotedStringTokenizer;
class TReqTokenizer;
struct TDeferredToken;
struct TLangPrefix;
struct TLangSuffix;
struct TLengthLimits;
int ParseYandexRequest(TReqTokenizer*);

struct TOpInfo;
struct TLengthLimits;

namespace NTokenizerVersionsInfo {
    extern const size_t Default;
    extern const size_t DefaultWithOperators;
    extern const size_t DefaultWithoutOperators;
    extern const size_t LegacyForQueryNormalizer;
};

//! builds binary syntax tree from a user request
class tRequest: private TNonCopyable
{
    friend class TQuotedStringTokenizer;
    friend class TReqTokenizer;
    template<size_t> friend class TVersionedReqTokenizer;
    friend int ParseYandexRequest(TReqTokenizer*);

private:
    const ui32 ProcessingFlag;
    const TOpInfo& PhraseOperInfo;
    TPhraseType DefaultType;

    //! the list of attributes that is used during parsing
    const TReqAttrList& AttrList;
    //! the default set of attributes that is used if @c tRequest is constructed with no attribute list
    static const TReqAttrList DefaultAttrList;

public:
    typedef TAutoPtr<TRequestNode> TNodePtr;

    //! @param flags            processing flags
    //! @param attrList         pointer to allowed request attribute list, if it is equal to NULL the default list is used
    explicit tRequest(ui32 flags = RPF_DEFAULT, const TReqAttrList* attrList = nullptr);

    //! builds request tree for request
    //! @param reqText          request to be parsed
    //! @param[out] foundReqLang whether an operator or other occurence of request language was met (REQWIZARD-1233)
    //! @throw TError           if request is invalid or parsing fails
    TNodePtr Parse(const TUtf16String& reqText, const TLengthLimits* limits = nullptr, bool* truncated = nullptr, size_t tokenizerVersion = NTokenizerVersionsInfo::Default, NSearchRequest::TOperatorsFound* foundReqLang = nullptr, bool postProcess = true) const;

    //! builds request tree for request and processes phrases in addition
    //! @param reqText          request to be parsed
    //! @param attrList         list of attributes, for example: "url:ATTR_URL,doc\ncat:ATTR_INTEGER,doc\ntitle:ZONE"
    //! @throw TError           if request is invalid or parsing fails
    TNodePtr Analyze(const wchar16* reqText,
                     const TLengthLimits* limits = nullptr, bool* truncated = nullptr, size_t tokenizerVersion = NTokenizerVersionsInfo::Default, NSearchRequest::TOperatorsFound* foundReqLang = nullptr) const;
    TNodePtr Analyze(const wchar16* reqText, const char* attrList,
                     const TLengthLimits* limits = nullptr, bool* truncated = nullptr, size_t tokenizerVersion = NTokenizerVersionsInfo::Default, NSearchRequest::TOperatorsFound* foundReqLang = nullptr) const;
    TNodePtr Analyze(const wchar16* reqText, const TReqAttrList* attrList,
                     const TLengthLimits* limits = nullptr, bool* truncated = nullptr, size_t tokenizerVersion = NTokenizerVersionsInfo::Default, NSearchRequest::TOperatorsFound* foundReqLang = nullptr) const;

    // getters
    ui32 GetProcessingFlag() const { return ProcessingFlag; }
    const TOpInfo& GetPhraseOperInfo() const { return PhraseOperInfo; }

    //! returns request attribute type for given name
    EReqAttrType GetAttrType(const TUtf16String& attrName) const {
        return AttrList.GetType(attrName, GetProcessingFlag());
    }
    ui8 GetNGrammBase(const TUtf16String& attrName) const {
        const TReqAttrList::TAttrData* data = AttrList.GetAttrData(attrName);
        if (!data)
            return 0;
        return data->NGrammBase;
    }
    bool IsDocTarg(const TUtf16String& attrName) const {
        const TReqAttrList::TAttrData* data = AttrList.GetAttrData(attrName);
        return data && data->Target == RA_TARG_DOC;
    }
    bool IsDocUrlTarg(const TUtf16String& attrName) const {
        const TReqAttrList::TAttrData* data = AttrList.GetAttrData(attrName);
        return data && data->Target == RA_TARG_DOCURL;
    }

    //! @note since zones and attributes in old style have different syntax it has @c IsKnownZone and @c IsKnownAttr
    bool IsKnownZone(const TUtf16String& name) const {
        return AttrList.IsZone(GetAttrType(name));
    }
    bool IsKnownAttr(const TUtf16String& name) const {
        return AttrList.IsAttr(GetAttrType(name));
    }

    bool UseTokenClassifier() const {
        return ProcessingFlag & RPF_USE_TOKEN_CLASSIFIER;
    }

    bool UseTrimExtraTokens() const {
        return ProcessingFlag & RPF_TRIM_EXTRA_TOKENS;
    }

private:
    void RunAnalyzer(TNodePtr& root) const;
};

// Функция собирает слова из запроса
void PickupWords(const TRequestNode* node, TVector<TString>& words, TVector<TString>& spaces);
void PickupWords(const TRequestNode* node, TVector<TUtf16String>& words, TVector<TUtf16String>& spaces);

// Функция производит разбиение запроса на слова
void CollectWords(const wchar16* request, TVector<TUtf16String>& words, TVector<TUtf16String>& spaces);
    //in YandexCodePage
void CollectWords(const char* request, TVector<TString>& words, TVector<TString>& spaces);

//! returns true if tokens classified and should be concatenated
bool ClassifyTokens(const wchar16* tokenBegin,
                    size_t length,
                    NTokenClassification::TTokenTypes& TokenTypes);

typedef std::pair<tRequest::TNodePtr, bool> TBinaryTreeInfo;


template <typename T>
TBinaryTreeInfo SimplyCreateBinaryTree(const TUtf16String& query, const T* attrList, ui32 reqflags,
                                       const TLengthLimits* limits = nullptr, size_t tokenizerVersion = NTokenizerVersionsInfo::Default, NSearchRequest::TOperatorsFound* foundReqLang = nullptr) {
    bool truncated = false;
    tRequest req(reqflags);
    tRequest::TNodePtr tree = req.Analyze(query.data(), attrList, limits, &truncated, tokenizerVersion, foundReqLang);
    return std::make_pair(tree, truncated);
}

template <typename T>
TBinaryTreeInfo CreateBinaryTree(const TUtf16String& query, const T* attrList, ui32 reqflags,
                                 const TLengthLimits* limits = nullptr, size_t tokenizerVersion = NTokenizerVersionsInfo::Default, NSearchRequest::TOperatorsFound* foundReqLang = nullptr) {
    try {
        return SimplyCreateBinaryTree(query, attrList, reqflags, limits, tokenizerVersion, foundReqLang);
    } catch (const TSyntaxError&) {
        if (reqflags & RPF_DONT_FIX_SYNTAX)
            throw;
    }
    TUtf16String fixedQuery = query;
    RemoveOperatorSymbols(fixedQuery);
    return SimplyCreateBinaryTree(fixedQuery, attrList, reqflags, limits, tokenizerVersion, foundReqLang);
}
