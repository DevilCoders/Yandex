#pragma once

/// Public API of remorph.
/// Describes functions and interfaces, which are available as DLL/SO implementation

namespace NRemorphAPI {

// Multitokens post-processing mode regulates how multitokens are split up (if split at all) after tokenization.
enum EMultitokenSplitMode {
    MSM_MINIMAL,    // Split only by minimal set of characters required for sane operation.
    MSM_SMART,      // Split all multitokens, except words-only tokens-with-hyphen, tokens'with'apostrophe.
    MSM_ALL,        // Split all multitokens.
};

enum ERankCheck {
    RC_G_COVERAGE,  // Greater coverage wins.
    RC_L_COUNT,     // Lesser facts number wins.
    RC_G_WEIGHT,    // Greater weight wins.
};

/// Base class for all remorph objects
struct IBase {
    /// Increments the internal count of references.
    /// @return current count of references after incrementation
    virtual long AddRef() = 0;
    /// Decrements the internal count of references and destroys the object if the count becomes zero
    /// @return current count of references after decrementation
    virtual long Release() = 0;
};

/// Data blob
struct IBlob: public virtual IBase {
    /// Returns pointer to blob data.
    /// @return pointer to blob data
    virtual const char* GetData() const = 0;
    /// Returns blob data size.
    /// @return blob data size
    virtual unsigned long GetSize() const = 0;
};

/// Gazetteer article
struct IArticle: public virtual IBase {
    /// Returns gazetteer type name of the article.
    /// The returned pointer is owned by the IArticle implementation and should not be deleted.
    virtual const char* GetType() const = 0;
    /// Returns article name.
    /// The returned pointer is owned by the IArticle implementation and should not be deleted.
    virtual const char* GetName() const = 0;
    /// Returns article data blob.
    /// @return blob object with initial reference count = 1
    virtual IBlob* GetBlob() const = 0;
    /// Returns article JSON blob.
    /// @return blob object with initial reference count = 1
    virtual IBlob* GetJsonBlob() const = 0;
};

/// Collection of gazetteer articles
struct IArticles: public virtual IBase {
    /// Returns count of articles in the collection
    virtual unsigned long GetArticleCount() const = 0;
    /// Returns article with the specified num
    /// @param num article number in the range 0 <= num < GetCount()
    /// @return article object with initial reference count = 1, or NULL for invalid article number
    virtual IArticle* GetArticle(unsigned long num) const = 0;
};

/// Range info.
/// Base interface for IToken and IField
struct IRange: public virtual IBase {
    /// Returns the start sentence offset (in characters) of the range
    virtual unsigned long GetStartSentPos() const = 0;
    /// Returns the exclusive end sentence offset (in characters) of the range
    virtual unsigned long GetEndSentPos() const = 0;
    /// Returns collection of gazetteer articles, which are assigned to the range
    /// @return article collection with initial reference count = 1
    virtual IArticles* GetArticles() const = 0;
    /// Checks that the range has the specified article
    /// @param name gazetteer type or article name
    /// @return true if the range has an article with the specified name or type
    virtual bool HasArticle(const char* name) const = 0;
};

/// Token.
/// Inherits the IRange interface
struct IToken: public virtual IRange {
    /// Returns the original token text.
    /// The returned pointer is owned by the IText implementation and should not be deleted.
    virtual const char* GetText() const = 0;
};

/// Collection of tokens
struct ITokens: public virtual IBase {
    /// Returns count of tokens in the collection
    virtual unsigned long GetTokenCount() const = 0;
    /// Returns token with the specified num, where the num corresponds to the token
    /// sequence number in the sentence.
    /// @param num token number in the range 0 <= num < GetCount()
    /// @return token object with initial reference count = 1, or NULL for invalid token number
    virtual IToken* GetToken(unsigned long num) const = 0;
};

/// Simple fact field.
/// Inherits the IRange interface
struct IField: public virtual IRange {
    /// Returns the field name.
    /// The returned pointer is owned by the IField implementation and should not be deleted.
    virtual const char* GetName() const = 0;
    /// Returns field value.
    /// The returned pointer is owned by the IField implementation and should not be deleted.
    virtual const char* GetValue() const = 0;
    /// Returns the start token number of the field
    virtual unsigned long GetStartToken() const = 0;
    /// Returns the exclusive end token number of the field
    virtual unsigned long GetEndToken() const = 0;
    /// Returns collection of field tokens
    /// Tokens in this collection are ordered by the appearance in the sentence.
    /// @return tokens collection with initial reference count = 1
    virtual ITokens* GetTokens() const = 0;
};

/// Collection of simple fields
struct IFields: public virtual IBase {
    /// Returns count of fields in the collection
    virtual unsigned long GetFieldCount() const = 0;
    /// Returns field with the specified num
    /// @param num field number in the range 0 <= num < GetFieldCount()
    /// @return field object with initial reference count = 1, or NULL for invalid field number
    virtual IField* GetField(unsigned long num) const = 0;
};

struct ICompoundFields;

/// Compound fact field, which may contain sub-fields
/// Inherits the IField interface
struct ICompoundField: public virtual IField {
    /// Returns the name of the rule, which have constructed this field.
    /// The returned pointer is owned by the ICompoundField implementation and should not be deleted.
    virtual const char* GetRule() const = 0;
    /// Returns the weight of the compound field
    virtual double GetWeight() const = 0;

    /// Returns collection of all simple sub-fields
    /// @return fields collection with initial reference count = 1
    virtual IFields* GetFields() const = 0;
    /// Returns collection of simple sub-fields with the specified name.
    /// In case of missing fields with the specified name, the empty collection is returned.
    /// @param fieldName sub-field name
    /// @return fields collection with initial reference count = 1
    virtual IFields* GetFields(const char* fieldName) const = 0;

    /// Returns collection of all compound sub-fields
    /// @return fields collection with initial reference count = 1
    virtual ICompoundFields* GetCompoundFields() const = 0;
    /// Returns collection of compound sub-fields with the specified name.
    /// In case of missing fields with the specified name, the empty collection is returned.
    /// @param fieldName sub-field name
    /// @return fields collection with initial reference count = 1
    virtual ICompoundFields* GetCompoundFields(const char* fieldName) const = 0;
};

/// Collection of compound fields
struct ICompoundFields: public virtual IBase {
    /// Returns count of compound fields in the collection
    virtual unsigned long GetCompoundFieldCount() const = 0;
    /// Returns compound field with the specified num
    /// @param num field number in the range 0 <= num < GetFieldCount()
    /// @return compound field object with initial reference count = 1, or NULL for invalid field number
    virtual ICompoundField* GetCompoundField(unsigned long num) const = 0;
};

/// Fact.
/// Inherits the ICompoundFields interface
struct IFact: public virtual ICompoundField {
    /// Returns name of the fact type (message type name in the proto-descriptor).
    /// The returned pointer is owned by the IFact implementation and should not be deleted.
    virtual const char* GetType() const = 0;
    /// Returns the fact coverage (in number of tokens)
    virtual unsigned long GetCoverage() const = 0;
};

/// Collection of facts
struct IFacts: public virtual IBase {
    /// Returns count of facts in the collection
    virtual unsigned long GetFactCount() const = 0;
    /// Returns fact with the specified num
    /// @param num fact number in the range 0 <= num < GetCount()
    /// @return fact object with initial reference count = 1, or NULL for invalid fact number
    virtual IFact* GetFact(unsigned long num) const = 0;
};

/// Collection of solutions
struct ISolutions: public virtual IBase {
    /// Returns count of solutions in the collection
    virtual unsigned long GetSolutionCount() const = 0;
    /// Returns collection of non-intersecting facts, which belongs to the specified solution number.
    /// Facts in the returned collection are ordered by the position.
    /// @param num solution number in the range 0 <= num < GetCount()
    /// @return facts collection with initial reference count = 1, or NULL for invalid collection number
    virtual IFacts* GetSolution(unsigned long num) const = 0;
};

/// Sentence processing result
struct ISentence: public virtual IBase {
    /// Returns the original sentence text.
    /// The returned pointer is owned by the ISentence implementation and should not be deleted.
    virtual const char* GetText() const = 0;
    /// Returns collection of sentence tokens
    /// Tokens in this collection are ordered by the appearance in the sentence.
    /// @return tokens collection with initial reference count = 1
    virtual ITokens* GetTokens() const = 0;
    /// Returns all found facts in the current sentence.
    /// Facts in the returned collection are _unordered_.
    /// @return facts collection with initial reference count = 1
    virtual IFacts* GetAllFacts() const = 0;
    /// Finds and returns the best default-ranked solution (collection of non-intersecting facts).
    /// Default ranking is performed by greater coverage, lesser items count and greater weight.
    /// @return facts collection with initial reference count = 1
    virtual IFacts* FindBestSolution() const = 0;
    /// Finds and returns the best ranked solution (collection of non-intersecting facts).
    /// Ranking method is specified by providing ordered sequence of ranking checks to be performed consequently.
    /// @param rankChecks ranking checks sequence to be used to determine solutions order
    /// @param rankChecksNum count of ranking checks in the specified checks sequence
    /// @return facts collection with initial reference count = 1
    virtual IFacts* FindBestSolution(const ERankCheck* rankChecks, unsigned int rankChecksNum) const = 0;
    /// Finds and returns collection of top default-ranked solutions.
    /// Default ranking is performed by greater coverage, lesser items count and greater weight.
    /// @param maxSolutions upper limit for solution count
    /// @return solutions collection with initial reference count = 1
    virtual ISolutions* FindAllSolutions(unsigned short maxSolutions) const = 0;
    /// Finds and returns collection of top ranked solutions.
    /// Ranking method is specified by providing ordered sequence of ranking checks to be performed consequently.
    /// @param maxSolutions upper limit for solution count
    /// @param rankChecks ranking checks sequence to be used to determine solutions order
    /// @param rankChecksNum count of ranking checks in the specified checks sequence
    /// @return solutions collection with initial reference count = 1
    virtual ISolutions* FindAllSolutions(unsigned short maxSolutions, const ERankCheck* rankChecks, unsigned int rankChecksNum) const = 0;
};

/// Collection of resulting sentences
struct IResults: public virtual IBase {
    /// Returns count of sentences in the collection
    virtual unsigned long GetSentenceCount() const = 0;
    /// Returns sentence result with the specified num
    /// @param num sentence number in the range 0 <= num < GetCount()
    /// @return sentence result object with initial reference count = 1, or NULL for invalid fact number
    virtual ISentence* GetSentence(unsigned long num) const = 0;
};

/// Remorph processor
struct IProcessor: public virtual IBase {
    /// Processes the specified text and creates collection of results for it
    /// @param text null-terminated string in UTF-8 encoding
    /// @return collection of resulting sentences with initial reference count = 1, or NULL if no detected sentences
    virtual IResults* Process(const char* text) const = 0;
};

/// Processor factory
struct IFactory: public virtual IBase {
    /// Creates the remorph processor object with the specified processing options
    /// @param protoPath path to the proto-file with fact description
    /// @return processor with initial reference count = 1 or NULL in case of any error
    virtual IProcessor* CreateProcessor(const char* protoPath) = 0;

    /// Gets languages used to process text
    /// @return comma-separated list of language codes
    virtual const char* GetLangs() const = 0;

    /// Sets languages used to process text
    /// @note default value: basic languages ("eng,rus,ukr").
    /// @param langs comma-separated list of language codes
    /// @return false in case of any error, otherwise true
    virtual bool SetLangs(const char* langs) = 0;

    /// Gets sentence detection flag
    /// @return false means passed text is treated as a single sentence
    virtual bool GetDetectSentences() const = 0;

    /// Enables or disables sentence detection
    /// @note default value: true
    /// @param detectSentences pass false to treat all passed text as a single sentence
    virtual void SetDetectSentences(bool detectSentences) = 0;

    /// Gets sentence length limit
    /// @return max number of tokens per sentence
    virtual unsigned long GetMaxSentenceTokens() const = 0;

    /// Sets sentence length limit
    /// @note default value: 100
    /// @param maxSentenceTokens max number of tokens per sentence, this parameter is ignored, when detectSentences=false
    virtual void SetMaxSentenceTokens(unsigned long maxSentenceTokens) = 0;

    /// Gets post-tokenization multitoken splitting mode
    /// @param multitokenSplitMode multitoken splitting mode
    virtual EMultitokenSplitMode GetMultitokenSplitMode() const = 0;

    /// Sets post-tokenization multitoken splitting mode
    /// @note default value: MSM_MINIMAL
    /// @param multitokenSplitMode multitoken splitting mode
    /// @return false in case of any error, otherwise true
    virtual bool SetMultitokenSplitMode(EMultitokenSplitMode multitokenSplitMode) = 0;

    // Returns error, which occurred in last CreateProcessor() method call, or NULL is there was no error
    virtual const char* GetLastError() const = 0;
};

/// General info
struct IInfo: public virtual IBase {
    /// Returns Remorph major version.
    /// @return major version
    virtual unsigned int GetMajorVersion() const = 0;

    /// Returns Remorph minor version.
    /// @return minor version
    virtual unsigned int GetMinorVersion() const = 0;

    /// Returns Remorph patch version.
    /// @return patch version
    virtual unsigned int GetPatchVersion() const = 0;
};

} // NRemorphAPI

typedef NRemorphAPI::IFactory* (*TFuncGetRemorphFactory)();

extern "C" {
    /// Creates factory object which is used to create processors
    /// @return factory with initial reference count = 1
    NRemorphAPI::IFactory* GetRemorphFactory();
    /// Creates info object which provides general information about remorph
    /// @return info with initial reference count = 1
    NRemorphAPI::IInfo* GetRemorphInfo();
}
