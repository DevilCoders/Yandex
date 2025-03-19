#pragma once

#include <kernel/qtree/richrequest/serialization/flags.h>

#include <util/generic/string.h>
#include <util/charset/wide.h>
#include <library/cpp/token/nlptypes.h>
#include <kernel/lemmer/core/wordinstance.h>
#include <library/cpp/stopwords/stopwords.h>

class TLanguageContext;

namespace NRichTreeProtocol {
    class TWordNode;
    class TLemma;
}

bool Compare(const TLemmaForms& a, const TLemmaForms& b);
bool SoftCompare(const TLemmaForms& a, const TLemmaForms& b);

//! join type of word in request
typedef ui8 TWordJoin;

enum EWordJoin {
    WORDJOIN_UNDEF = 0x0,
    WORDJOIN_NONE = 0x1,
    WORDJOIN_DELIM = 0x2,
    WORDJOIN_NODELIM = 0x4,
    WORDJOIN_DEFAULT = (WORDJOIN_NONE | WORDJOIN_DELIM)
};

class TWordNode: public TWordInstance {
private:
    enum {
        LEM_TYPE_NON_LEMMER = 0x0,
        LEM_TYPE_LEMMER_WORD = 0x1,
        LEM_TYPE_INTEGER = 0x2,
    };
private:
    int LemType;

    long RevFr;
    double UpDivLo;

private:
    TWordNode();
    static TAutoPtr<TWordNode> CreateNonLemmerNodeInt(const TUtf16String& lemma, const TUtf16String& forma, TFormType ftype, int lemType);
    static TAutoPtr<TWordNode> CreateLemmerNodeInt(const TUtf16String& word,
                                                  const TCharSpan& span,
                                                  TFormType ftype,
                                                  const TLanguageContext& lang,
                                                  bool generateForms,
                                                  bool useFixList);

public:
    static TAutoPtr<TWordNode> CreateEmptyNode();

    // Used in TBoostingWizard.
    static TAutoPtr<TWordNode> CreateLemmerNodeWithoutFixList(const TUtf16String& word, const TCharSpan& span, TFormType ftype, const TLanguageContext& lang, bool generateForms);

    static TAutoPtr<TWordNode> CreateLemmerNode(const TUtf16String& word, const TCharSpan& span, TFormType ftype, const TLanguageContext& lang, bool generateForms = true);
    static TAutoPtr<TWordNode> CreateLemmerNode(const TVector<const TYandexLemma*>& lms, TFormType ftype, const TLanguageContext& lang, bool generateForms = true);
    static TAutoPtr<TWordNode> CreateNonLemmerNode(const TUtf16String& forma, TFormType ftype);
    static TAutoPtr<TWordNode> CreateAttributeIntegerNode(const TUtf16String& forma, TFormType ftype);
    static TAutoPtr<TWordNode> CreatePlainIntegerNode(const TUtf16String& forma, TFormType ftype); // for yserver
    static TAutoPtr<TWordNode> CreateTextIntegerNode(const TUtf16String& forma, TFormType ftype, bool planeInteger = false);

    bool Compare(const TWordNode& other) const;
    void Serialize(NRichTreeProtocol::TWordNode& message, bool humanReadable) const;
    void Deserialize(const NRichTreeProtocol::TWordNode& message, EQtreeDeserializeMode mode);

    TFormType GetFormType() const;
    void SetFormType(TFormType formType);

    bool IsLemmerWord() const {
        return LemType & LEM_TYPE_LEMMER_WORD;
    }

    bool IsInteger() const {
        return LemType & LEM_TYPE_INTEGER;
    }

    void SetRevFr(long revFr) {
        RevFr = revFr;
    }
    long GetRevFr() const {
        return RevFr;
    }

    void SetUpDivLoFreq(double upDivLo) {
        UpDivLo = upDivLo;
    }
    double GetUpDivLoFreq() const {
        return UpDivLo;
    }

private:
    using TWordInstance::Init;
};
