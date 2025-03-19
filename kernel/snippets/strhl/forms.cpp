#include <library/cpp/charset/recyr.hh>
#include <library/cpp/charset/ci_string.h>
#include <library/cpp/charset/wide.h>
#include <library/cpp/containers/atomizer/atomizer.h>
#include <library/cpp/containers/str_hash/str_hash.h>

#include <library/cpp/tokenizer/tokenizer.h>
#include <kernel/lemmer/core/language.h>

#include <kernel/qtree/request/req_node.h>
#include <kernel/qtree/request/request.h>

#include <util/generic/noncopyable.h>

#include "forms.h"

static void collectGoodForms(const TRequestNode *n, TVector<TString> &words)
{
   if (!n)
      return;
   if (n->IsLeaf() && IsWord(*n)) {
       words.push_back(WideToChar(n->GetText(), CODES_YANDEX));
   } else {
      collectGoodForms(n->Left, words);
      if (n->Op() != oAndNot)
         collectGoodForms(n->Right, words);
   }
}

static void collectAllForms(const TRequestNode *n, TVector<TString> &words, bool isNegator)
{
   if (!n)
      return;
   if (n->IsLeaf() && IsWord(*n)) {
      words.push_back(WideToChar(n->GetText(), CODES_YANDEX));
      if (n->FormType == fExactWord)
          words.back().prepend("!");
      if (isNegator)
          words.back().prepend("-");
   } else {
      collectAllForms(n->Left, words, isNegator);
      collectAllForms(n->Right, words, isNegator || n->Op() == oAndNot);
   }
}

void Request2Forms(const char* request, TVector<TString> &words, bool collectMinuWords)
{
    THolder<TRequestNode> root;
    try {
        const TUtf16String s = CharToWide(request, csYandex);
        root.Reset(tRequest().Analyze(s.data()).Release());
    } catch (...) {
        return;
    }

    words.reserve(10);
    if (collectMinuWords)
        collectAllForms(root.Get(), words, false);
    else
        collectGoodForms(root.Get(), words);
}

inline TString GetLemmaText(const TYandexLemma* lemm) {
    return WideToChar(lemm->GetText(), lemm->GetTextLength(), CODES_YANDEX);
}

static void Forms2Lemms(TVector<TString> &forms, TVector<TString> &lemms, LemmataGlueMode gluelemms, const HashSet* stopWords)
{
    lemms.reserve(10);
    TWLemmaArray la;
    for (size_t i = 0; i < forms.size(); ++i) {
        const TString& form = forms[i];
        if (stopWords && stopWords->Has(form.data()))
            continue;
        TUtf16String wform = CharToWide(form, csYandex);
        NLemmer::AnalyzeWord(wform.data(), wform.size(), la, LI_BASIC_LANGUAGES);
        TWUniqueLemmaArray uniques;
        FillUniqLemmas(uniques, la);
        if (gluelemms == LGM_UNDERLINE) {
            TString all_words;
            for (size_t j = 0; j < uniques.size(); j++)
                all_words = all_words + (j ? "_" : "") + GetLemmaText(uniques[j]);
            lemms.push_back(all_words);
        } else if (gluelemms == LGM_CURLY) {
            TString all_words;
            for (size_t j = 0; j < uniques.size(); j++) {
                const TYandexLemma* cl = uniques[j];
                all_words = all_words + (j ? "|" : "{") + GetLemmaText(cl);
                if (cl->IsBastard())
                    all_words += "?";
                if (cl->GetLanguage() == LANG_UNK)
                    all_words += "?";
            }
            all_words += "}";
            lemms.push_back(all_words);
        } else {
            for (size_t j = 0; j < uniques.size(); j++)
                lemms.push_back(GetLemmaText(uniques[j]));
        }
    }
}

void Request2Lemms(const char* request, TVector<TString> &lemms, LemmataGlueMode gluelemms, const HashSet* stopWords)
{
    TVector<TString> forms;
    Request2Forms(request, forms, false);
    Forms2Lemms(forms, lemms, gluelemms, stopWords);
}

static TString Words2Stroka(TVector<TString> &words)
{
   atomizer<ci_hash, ci_equal_to> WordHash(words.size(), 0);
   TString res;
   int wc = 0;
   for (size_t i = 0; i < words.size(); ++i) {
      TString &s = words[i];
      if (s.size() >= 1 && s.at(s.size() - 1) == '\x01')
         s.resize(s.size() - 1);
      if (!WordHash.find_atom(s.data())) {
         if (wc)
            res += " ";
         res += words[i];
         wc++;
         WordHash.perm_string_to_atom(s.data());
      }
   }
   return res;
}

//-- utility for feedbackSearch and banner rotator by words
//-- do not collects words under and_not operator
TString GetGoodWords(const char* request, const HashSet* stopWords)
{
    TVector<TString> lemms;
    Request2Lemms(request, lemms, LGM_NONE, stopWords);
    return Words2Stroka(lemms);
}

//-- utility for new (per-show) advq
//-- does not collect words under and_not operator
TString GetGoodWordsADVQ(const char* request, const HashSet* stopWords)
{
    TVector<TString> lemms;
    Request2Lemms(request, lemms, LGM_UNDERLINE, stopWords);
    return Words2Stroka(lemms);
}

TString GetGoodWordsK(const char* request, const HashSet* stopWords)
{
    TVector<TString> lemms;
    Request2Lemms(request, lemms, LGM_CURLY, stopWords);
    for (TVector<TString>::iterator i = lemms.begin(); i != lemms.end(); ++i) {
        char *b = i->begin();
        char* r = std::remove(b, b+i->length(), '\x01');
        i->resize(r-b);
    }
    return Words2Stroka(lemms);
}

//--
TString GetGoodForms(const char* request, bool collectMinusWords)
{
    TVector<TString> forms;
    Request2Forms(request, forms, collectMinusWords);
    return Words2Stroka(forms);
}

class TLemmCountHandler : public ITokenHandler, TNonCopyable
{
public:
    TLemmCountHandler(const TInlineHighlighter &ih)
      : IH(ih)
      , Count(0)
    {
    }

    void OnToken(const TWideToken& tok, size_t /*origleng*/, NLP_TYPE type) override {
        switch (type) {
            case NLP_WORD:
            case NLP_MARK:
            case NLP_INTEGER:
            case NLP_FLOAT:
                if (IH.IsGoodWord(tok))
                    Count++;
                break;
            default:
                break;
        }
    }

    size_t GetLemmCount() const {
        return Count;
    }

private:
    const TInlineHighlighter &IH;
    size_t Count;
};

size_t LemmCount(const TInlineHighlighter& ih, const char* text, size_t textLen)
{
    Y_ASSERT(text && textLen);

    TLemmCountHandler lcHandler(ih);
    TNlpTokenizer toker(lcHandler);

    TTempBuf wbuf((textLen + 1) * sizeof(wchar16));
    CharToWide(text, (unsigned int)textLen, (wchar16*)wbuf.Data(), csYandex);
    *(((wchar16*)wbuf.Data()) + textLen) = 0;
    toker.Tokenize((const wchar16*)wbuf.Data(), textLen);

    return lcHandler.GetLemmCount();
}
