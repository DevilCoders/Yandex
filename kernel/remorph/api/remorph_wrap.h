#pragma once

// Declares wrapper objects with automatic life-time management for all interfaces, which are defined in "remorph.h"
// This file should be compilable by old compilers (lucid stock gcc)

#include "remorph.h"

#include <exception>

#define DECLARE_COPY_CTR_ASSIGN_OP(T) \
    inline T(const T& obj) noexcept { \
        Assign(obj); \
    } \
    inline T& operator =(const T& obj) noexcept { \
        Assign(obj); \
        return *this; \
    }

namespace NRemorphAPI {

class TNullPtrException: public std::exception {
public:
    virtual const char* what() const noexcept {
        return "null ptr";
    }
};

/// Base class for all remorph objects
template <class T>
class TPtrWrap {
public:
    inline TPtrWrap(T* p = 0) noexcept
        : Ptr(p)
    {
    }

    inline ~TPtrWrap() {
        Reset();
    }

    template <class Y>
    void Assign(const TPtrWrap<Y>& obj) noexcept {
        T* old = Ptr;
        Ptr = obj.Ptr;
        if (Ptr) {
            Ptr->AddRef();
        }
        if (old) {
            old->Release();
        }
    }

    inline bool IsValid() const noexcept {
        return 0 != Ptr;
    }

    inline void Reset() noexcept {
        if (Ptr) {
            Ptr->Release();
        }
        Ptr = 0;
    }

    DECLARE_COPY_CTR_ASSIGN_OP(TPtrWrap<T>);

protected:
    inline T* Get() {
        if (!IsValid())
            throw TNullPtrException();
        return Ptr;
    }

    inline const T* Get() const {
        if (!IsValid())
            throw TNullPtrException();
        return Ptr;
    }

private:
    T* Ptr = nullptr;
};

struct TBlobWrap: public TPtrWrap<IBlob> {
    inline TBlobWrap(IBlob* p = 0) noexcept
        : TPtrWrap<IBlob>(p)
    {
    }
    inline const char* GetData() const {
        return Get()->GetData();
    }
    inline unsigned long GetSize() const {
        return Get()->GetSize();
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TBlobWrap)
};

struct TArticleWrap: public TPtrWrap<IArticle> {
    inline TArticleWrap(IArticle* p = 0) noexcept
        : TPtrWrap<IArticle>(p)
    {
    }
    inline const char* GetType() const {
        return Get()->GetType();
    }
    inline const char* GetName() const {
        return Get()->GetName();
    }
    inline TBlobWrap GetBlob() const {
        return Get()->GetBlob();
    }
    inline TBlobWrap GetJsonBlob() const {
        return Get()->GetJsonBlob();
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TArticleWrap)
};

struct TArticlesWrap: public TPtrWrap<IArticles> {
    inline TArticlesWrap(IArticles* p = 0) noexcept
        : TPtrWrap<IArticles>(p)
    {
    }
    inline unsigned long GetArticleCount() const {
        return Get()->GetArticleCount();
    }
    inline TArticleWrap GetArticle(unsigned long num) const {
        return Get()->GetArticle(num);
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TArticlesWrap)
};

template <class T>
struct TRangeWrap: public TPtrWrap<T> {
    inline TRangeWrap(T* p = 0) noexcept
        : TPtrWrap<T>(p)
    {
    }
    inline unsigned long GetStartSentPos() const {
        return TPtrWrap<T>::Get()->GetStartSentPos();
    }
    inline unsigned long GetEndSentPos() const {
        return TPtrWrap<T>::Get()->GetEndSentPos();
    }
    inline TArticlesWrap GetArticles() const {
        return TPtrWrap<T>::Get()->GetArticles();
    }
    inline bool HasArticle(const char* name) const {
        return TPtrWrap<T>::Get()->HasArticle(name);
    }
};

struct TTokenWrap: public TRangeWrap<IToken> {
    inline TTokenWrap(IToken* p = 0) noexcept
        : TRangeWrap<IToken>(p)
    {
    }
    inline const char* GetText() const {
        return Get()->GetText();
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TTokenWrap)
};

struct TTokensWrap: public TPtrWrap<ITokens> {
    inline TTokensWrap(ITokens* p = 0) noexcept
        : TPtrWrap<ITokens>(p)
    {
    }
    inline unsigned long GetTokenCount() const {
        return Get()->GetTokenCount();
    }
    inline TTokenWrap GetToken(unsigned long num) const {
        return Get()->GetToken(num);
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TTokensWrap)
};

template <class T>
struct TFieldWrapBase: public TRangeWrap<T> {
    inline TFieldWrapBase(T* p = 0) noexcept
        : TRangeWrap<T>(p)
    {
    }
    inline const char* GetName() const {
        return TRangeWrap<T>::Get()->GetName();
    }
    inline const char* GetValue() const {
        return TRangeWrap<T>::Get()->GetValue();
    }
    inline unsigned long GetStartToken() const {
        return TRangeWrap<T>::Get()->GetStartToken();
    }
    inline unsigned long GetEndToken() const {
        return TRangeWrap<T>::Get()->GetEndToken();
    }
    inline TTokensWrap GetTokens() const {
        return TRangeWrap<T>::Get()->GetTokens();
    }
};

struct TFieldWrap: public TFieldWrapBase<IField> {
    inline TFieldWrap(IField* p = 0) noexcept
        : TFieldWrapBase<IField>(p)
    {
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TFieldWrap)
};

struct TFieldsWrap: public TPtrWrap<IFields> {
    inline TFieldsWrap(IFields* p = 0) noexcept
        : TPtrWrap<IFields>(p)
    {
    }
    inline unsigned long GetFieldCount() const {
        return Get()->GetFieldCount();
    }
    inline TFieldWrap GetField(unsigned long num) const {
        return Get()->GetField(num);
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TFieldsWrap)
};

struct TCompoundFieldsWrap;

template <class T>
struct TCompoundFieldWrapBase: public TFieldWrapBase<T> {
    inline TCompoundFieldWrapBase(T* p = 0) noexcept
        : TFieldWrapBase<T>(p)
    {
    }
    inline const char* GetRule() const {
        return TFieldWrapBase<T>::Get()->GetRule();
    }
    inline double GetWeight() const {
        return TFieldWrapBase<T>::Get()->GetWeight();
    }
    inline TFieldsWrap GetFields() const {
        return TFieldWrapBase<T>::Get()->GetFields();
    }
    inline TFieldsWrap GetFields(const char* fieldName) const {
        return TFieldWrapBase<T>::Get()->GetFields(fieldName);
    }
    TCompoundFieldsWrap GetCompoundFields() const;
    TCompoundFieldsWrap GetCompoundFields(const char* fieldName) const;
};

struct TCompoundFieldWrap: public TCompoundFieldWrapBase<ICompoundField> {
    inline TCompoundFieldWrap(ICompoundField* p = 0) noexcept
        : TCompoundFieldWrapBase<ICompoundField>(p)
    {
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TCompoundFieldWrap)
};

struct TCompoundFieldsWrap: public TPtrWrap<ICompoundFields> {
    inline TCompoundFieldsWrap(ICompoundFields* p = 0) noexcept
        : TPtrWrap<ICompoundFields>(p)
    {
    }
    inline unsigned long GetCompoundFieldCount() const {
        return Get()->GetCompoundFieldCount();
    }
    inline TCompoundFieldWrap GetCompoundField(unsigned long num) const {
        return Get()->GetCompoundField(num);
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TCompoundFieldsWrap)
};

template <class T>
inline TCompoundFieldsWrap TCompoundFieldWrapBase<T>::GetCompoundFields() const {
    return TFieldWrapBase<T>::Get()->GetCompoundFields();
}

template <class T>
inline TCompoundFieldsWrap TCompoundFieldWrapBase<T>::GetCompoundFields(const char* fieldName) const {
    return TFieldWrapBase<T>::Get()->GetCompoundFields(fieldName);
}

struct TFactWrap: public TCompoundFieldWrapBase<IFact> {
    inline TFactWrap(IFact* p = 0) noexcept
        : TCompoundFieldWrapBase<IFact>(p)
    {
    }
    inline const char* GetType() const {
        return Get()->GetType();
    }
    inline unsigned long GetCoverage() const {
        return Get()->GetCoverage();
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TFactWrap)
};

struct TFactsWrap: public TPtrWrap<IFacts> {
    inline TFactsWrap(IFacts* p = 0) noexcept
        : TPtrWrap<IFacts>(p)
    {
    }
    inline unsigned long GetFactCount() const {
        return Get()->GetFactCount();
    }
    inline TFactWrap GetFact(unsigned long num) const {
        return Get()->GetFact(num);
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TFactsWrap)
};

struct TSolutionsWrap: public TPtrWrap<ISolutions> {
    inline TSolutionsWrap(ISolutions* p = 0) noexcept
        : TPtrWrap<ISolutions>(p)
    {
    }
    inline unsigned long GetSolutionCount() const {
        return Get()->GetSolutionCount();
    }
    inline TFactsWrap GetSolution(unsigned long num) const {
        return Get()->GetSolution(num);
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TSolutionsWrap)
};

struct TSentenceWrap: public TPtrWrap<ISentence> {
    inline TSentenceWrap(ISentence* p = 0) noexcept
        : TPtrWrap<ISentence>(p)
    {
    }
    inline const char* GetText() const {
        return Get()->GetText();
    }
    inline TTokensWrap GetTokens() const {
        return Get()->GetTokens();
    }
    inline TFactsWrap GetAllFacts() const {
        return Get()->GetAllFacts();
    }
    inline TFactsWrap FindBestSolution() const {
        return Get()->FindBestSolution();
    }
    inline TFactsWrap FindBestSolution(const ERankCheck* rankChecks, unsigned int rankChecksNum) const {
        return Get()->FindBestSolution(rankChecks, rankChecksNum);
    }
    inline TSolutionsWrap FindAllSolutions(unsigned short maxSolutions) const {
        return Get()->FindAllSolutions(maxSolutions);
    }
    inline TSolutionsWrap FindAllSolutions(unsigned short maxSolutions, const ERankCheck* rankChecks, unsigned int rankChecksNum) const {
        return Get()->FindAllSolutions(maxSolutions, rankChecks, rankChecksNum);
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TSentenceWrap)
};

struct TResultsWrap: public TPtrWrap<IResults> {
    inline TResultsWrap(IResults* p = 0) noexcept
        : TPtrWrap<IResults>(p)
    {
    }
    inline unsigned long GetSentenceCount() const {
        return Get()->GetSentenceCount();
    }
    inline TSentenceWrap GetSentence(unsigned long num) const {
        return Get()->GetSentence(num);
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TResultsWrap)
};

struct TProcessorWrap: public TPtrWrap<IProcessor> {
    inline TProcessorWrap(IProcessor* p = 0) noexcept
        : TPtrWrap<IProcessor>(p)
    {
    }
    inline TResultsWrap Process(const char* text) const {
        return Get()->Process(text);
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TProcessorWrap)
};

struct TFactoryWrap: public TPtrWrap<IFactory> {
    inline TFactoryWrap(IFactory* p = 0) noexcept
        : TPtrWrap<IFactory>(p)
    {
    }
    inline TProcessorWrap CreateProcessor(const char* protoPath) {
        return Get()->CreateProcessor(protoPath);
    }
    inline const char* GetLangs() const {
        return Get()->GetLangs();
    }
    inline bool SetLangs(const char* langs) {
        return Get()->SetLangs(langs);
    }
    inline bool GetDetectSentences() const {
        return Get()->GetDetectSentences();
    }
    inline void SetDetectSentences(bool detectSentences) {
        Get()->SetDetectSentences(detectSentences);
    }
    inline unsigned long GetMaxSentenceTokens() const {
        return Get()->GetMaxSentenceTokens();
    }
    inline void SetMaxSentenceTokens(unsigned long maxSentenceTokens) {
        Get()->SetMaxSentenceTokens(maxSentenceTokens);
    }
    inline EMultitokenSplitMode GetMultitokenSplitMode() const {
        return Get()->GetMultitokenSplitMode();
    }
    inline bool SetMultitokenSplitMode(EMultitokenSplitMode multitokenSplitMode) {
        return Get()->SetMultitokenSplitMode(multitokenSplitMode);
    }
    inline const char* GetLastError() const {
        return Get()->GetLastError();
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TFactoryWrap)
};

struct TInfoWrap: public TPtrWrap<IInfo> {
    inline TInfoWrap(IInfo* p = 0) noexcept
        : TPtrWrap<IInfo>(p)
    {
    }
    inline unsigned int GetMajorVersion() const {
        return Get()->GetMajorVersion();
    }
    inline unsigned int GetMinorVersion() const {
        return Get()->GetMinorVersion();
    }
    inline unsigned int GetPatchVersion() const {
        return Get()->GetPatchVersion();
    }
    DECLARE_COPY_CTR_ASSIGN_OP(TInfoWrap)
};

inline TFactoryWrap GetRemorphFactoryWrap() {
    return GetRemorphFactory();
}

inline TInfoWrap GetRemorphInfoWrap() {
    return GetRemorphInfo();
}

} // NRemorphAPI
