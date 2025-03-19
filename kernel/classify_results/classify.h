#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include "script.h"
#include "snippet_reader.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TObjectCategory {
    TUtf16String CategoryName;
    TUtf16String CanonicName;
    int operator& (IBinSaver &f) { f.Add(2, &CategoryName); f.Add(3, &CanonicName); return 0; }
};
typedef THashMap<TUtf16String, TVector<TObjectCategory> > TCategorizedReqsData;
////////////////////////////////////////////////////////////////////////////////////////////////////
class TCategorizedReqs
{
    TCategorizedReqsData Data;
    typedef THashMap<TUtf16String, TString> TObject2Comments;
    typedef THashMap<TUtf16String, TObject2Comments> TCateg2Comments;
    TCateg2Comments Categ2Object2Comment;
public: // debug and system
    int operator& (IBinSaver &f) { f.Add(2,&Data); return 0; }
    void CheckCorrectness() const;

public:
    void Load(const TString &filePath);
    void LoadOneObject(const TVector<TString>& line);
    void Save(const TString &filePath) const;
    void AddComment(const TUtf16String &categ, const TUtf16String &object, const TString &comment);
    TString GetComments(const TUtf16String &categ, const TUtf16String &object) const;

    typedef TCategorizedReqsData::iterator iterator;
    typedef TCategorizedReqsData::const_iterator const_iterator;
    const_iterator find(const TUtf16String &req) const {
        return Data.find(req);
    }
    iterator find(const TUtf16String &req) {
        return Data.find(req);
    }
    const_iterator begin() const {
        return Data.begin();
    }
    const_iterator end() const {
        return Data.end();
    }
    size_t size() const {
        return Data.size();
    }
    TVector<TObjectCategory>& operator[](const TUtf16String &req) {
        return Data[req];
    }
    const TVector<TObjectCategory>& operator[](const TUtf16String &req) const {
        return Data.find(req)->second;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TRephrase
{
    TUtf16String Category;
    TUtf16String Object; // object name as in user request
    TUtf16String ObjectCanonic; // canonic object name (in most cases it's equal to Object; could differ in case of typos, translit etc.)
    TUtf16String Intent; // intent UID (if no UID specified in script file, then first its formulation)
    bool IsForward; // true if request is [intent object], false otherwise
    int Props; // set of flags from EIntentProps
    TRephrase() {}
    TRephrase(const TUtf16String &cat, const TUtf16String &obj, const TUtf16String &objCan, const TUtf16String &intent, bool isForward, int props)
        : Category(cat)
        , Object(obj)
        , ObjectCanonic(objCan)
        , Intent(intent)
        , IsForward(isForward)
        , Props(props)
    {
    }
    int operator&(IBinSaver &f) { f.Add(2,&Category); f.Add(3, &Object); f.Add(4, &ObjectCanonic); f.Add(5,&Intent); f.Add(6,&Props); return 0; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class TRephrases
{
    enum EMatchFlag {
        MF_OPTIONAL = 0,
        MF_OBJECT = 1,
        MF_INTENT = 2
    };
    struct TRephraseData {
        THashMap<TUtf16String, EMatchFlag> Subphrases;
        THashMap<TUtf16String, TUtf16String> CanonicNames;
        TUtf16String Categ;
        TUtf16String Intent;
        int Props;
    };
    TVector<TRephraseData> RephraseTypes;
    typedef THashMap<TUtf16String, TVector<size_t> > TPhraseStart2Indices;
    TPhraseStart2Indices PhraseStart2Indices;

private:
    void AddIntents(const TClassifierScript &script);
    void FinishInitialization();

public: // common way
    void Fill(const TClassifierScript &script, const TCategorizedReqs &reqs);
    void Recognize(const TUtf16String &req, TVector<TRephrase> *res) const;

public: // advanced: objects list is not known beforehand, must use an external objects provider
    struct IObjectRecognizer {
        virtual ~IObjectRecognizer() {}
        virtual bool IsObjectFromCateg(const TUtf16String &obj, const TUtf16String &categ) const = 0;
    };
    void Fill(const TClassifierScript &script);
    void RecognizeExt(const TUtf16String &req, IObjectRecognizer *extObjects, TVector<TRephrase> *res) const;
    void RecognizeEmptyObject(const TUtf16String &req, TVector<TRephrase> *res) const;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class TExplicitIntentsDetector {
    TSearcherAutomaton<int, true> Fragment2Uid;
    TVector<TString> Uids;
public:
    void Fill(const TClassifierScript &script);
    bool RecognizeExplicitIntent(const TStringBuf &reqUtf, TString *uid) const;

    typedef TVector<bool> TSubset;
    void AddToRecognizeSubset(const TStringBuf &intentName, TSubset *res) const;
    void RecognizeAll(const TStringBuf &reqUtf, const TSubset &subset, TVector<TString> *res) const;
};
