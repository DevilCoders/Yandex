#include <tools/segutils/tests/tests_common/tests_common.h>
#include <kernel/segutils/numerator_utils.h>

using namespace NSegm;

namespace NSegutils {

struct TSegmentatorTest: TTest {
    THolder<TSegmentatorContext> SegCtx;

    void DoInit() override {
        SegCtx.Reset(new TSegmentatorContext(*Ctx));
    }

    TString ProcessDoc(const THtmlFile& f) override {
        TSegmentatorContext& segctx = *SegCtx;
        segctx.SetDoc(f);
        segctx.NumerateDoc();

        TEventStorage& st = segctx.GetEvents(false);

        {
            const THeaderSpans& hs = segctx.GetHeaderSpans();
            for (THeaderSpans::const_iterator it = hs.begin(); it != hs.end(); ++it)
                st.InsertSpan(*it, "Header");
        }

        {
            const TMainHeaderSpans& hs = segctx.GetMainHeaderSpans();
            for (TMainHeaderSpans::const_iterator it = hs.begin(); it != hs.end(); ++it)
                st.InsertSpan(*it, "MainHeader");
        }

        {
            const TSegmentSpans& ss = segctx.GetSegmentSpans();
            for (TSegmentSpans::const_iterator it = ss.begin(); it != ss.end(); ++it)
                st.InsertSpan(*it, Sprintf("Segment-%s", GetSegmentName(ESegmentType(it->Type))), UTF8ToWide(it->ToString(true)));
        }

        {
            const TMainContentSpans& ms = segctx.GetMainContentSpans();
            for (TMainContentSpans::const_iterator it = ms.begin(); it != ms.end(); ++it)
                st.InsertSpan(*it, "MainContent");
        }

        {
            const TArticleSpans& as = segctx.GetArticleSpans();
            for (TArticleSpans::const_iterator it = as.begin(); it != as.end(); ++it)
                st.InsertSpan(*it, "Article");
        }

        TString res;
        TStringOutput sout(res);
        sout << "<-------------------------------------------------->" << '\n';
        sout << "Url: " << f.Url << '\n';
        sout << "File: " << TFsPath(f.FileName).GetName() << '\n';
        sout << "<Title>" << '\n';
        PrintEvents(segctx.GetEvents(true), sout);
        sout << "</Title>" << '\n';
        sout << "<Body>" << '\n';
        PrintEvents(st, sout);
        sout << "</Body>" << '\n';
        sout << Endl;
        return res;
    }
};

}

int main(int argc, const char** argv) {
    using namespace NSegutils;
    TSegmentatorTest test;
    test.Init(argc, argv);
    test.RunTest();
}
