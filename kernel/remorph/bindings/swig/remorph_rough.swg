#define SWIGWORDSIZE64
%include <typemaps.i>
%include <bindings/swiglib/defaults.swg>
%include <bindings/swiglib/stroka.swg>
%include <bindings/swiglib/wtroka.swg>
%{
#include <kernel/remorph/api/remorph_wrap.h>
#include <util/string/printf.h>
#include <util/string/builder.h>
#include <util/generic/yexception.h>
#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>
using namespace NRemorphAPI;
%}

//using namespace NRemorphAPI;
class TFieldsWrap   {
};

class TProcessorWrap   {
    public:
%extend {
    TUtf16String FindImpl(const TUtf16String& text, bool find_all) const {
        const TString textUTF = WideToUTF8(text);
        TResultsWrap res = self->Process(textUTF.data());
        size_t sentCount = res.GetSentenceCount();
        TStringBuilder res_json;
        res_json << "[";
        for (size_t i = 0; i < sentCount; i++) {
            if (i > 0)
                res_json << ",";
            TSentenceWrap sent = res.GetSentence(i);
            res_json << "{ \"sent\":" << i << ", \"facts\": [";
            TFactsWrap fw =  sent.FindBestSolution();
            for (size_t k = 0; k < fw.GetFactCount(); k++) {
                TFactWrap fact = fw.GetFact(k);
                if (k > 0)
                    res_json << ",";
                res_json << "{\"name\": " << TString(fact.GetName()).Quote()
                         << ",\"start\": " << fact.GetStartToken()
                         << ",\"end\": " << fact.GetEndToken();
                TFieldsWrap fields = fact.GetFields();

                res_json << ",\"rule\": " << TString(fact.GetRule()).Quote() << " ";

                for (size_t j = 0; j < fields.GetFieldCount(); j++) {
                    res_json << ",";
                    if (find_all) {
                        TArticlesWrap articles = fields.GetField(j).GetArticles();
                        res_json << TString(fields.GetField(j).GetName()).Quote() << ": [";
                        for (size_t l = 0; l < articles.GetArticleCount(); ++l) {
                            if (l > 0)
                                res_json << ", ";
                            res_json << "\"" << NEscJ::EscapeJ<false, false>(articles.GetArticle(l).GetName()) << "\" ";
                        }
                        res_json << "]";
                    } else {
                        res_json << TString(fields.GetField(j).GetName()).Quote() << ": ";
                        res_json << "\"" << NEscJ::EscapeJ<false, false>(fields.GetField(j).GetValue()) << "\" ";
                    }
                }
                res_json << "}";
            }
            res_json << "]}";

        }
        res_json << "]";
        return UTF8ToWide(res_json);
    }

    void  Init(const char* filePath) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        factory.SetLangs("ru,uk,en");
        factory.SetDetectSentences(true);
        factory.SetMaxSentenceTokens(100u);
        factory.SetMultitokenSplitMode(MSM_SMART);
        TProcessorWrap p = factory.CreateProcessor(filePath);
        if ( ! p.IsValid() )
            ythrow yexception() << Sprintf("Cannot load file %s %s", filePath, factory.GetLastError());
        self->Assign( p );
    }

    TUtf16String Find(const TUtf16String& text) const {
        return TProcessorWrap_FindImpl(self, text, false);
    }

    TUtf16String FindAll(const TUtf16String& text) const {
        return TProcessorWrap_FindImpl(self, text, true);
    }

    virtual ~TProcessorWrap() {
        self->Reset();
    }
}
};


