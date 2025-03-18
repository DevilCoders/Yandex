#include <tools/segutils/tests/tests_common/tests_common.h>
#include <kernel/segutils/numerator_utils.h>

using namespace NSegm;

namespace NSegutils {

struct TStructZonesTest: TTest {
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
            const TSpans& spans = segctx.GetSegHandler()->GetTableCellSpans();
            for(TSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
                st.InsertSpan(*it, "TableCell");
            }
        }

        {
            const TSpans& spans = segctx.GetSegHandler()->GetTableRowSpans();
            for(TSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
                st.InsertSpan(*it, "TableRow");
            }
        }

        {
            const TSpans& spans = segctx.GetSegHandler()->GetTableSpans();
            for(TSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
                st.InsertSpan(*it, "Table");
            }
        }

        {
            const TTypedSpans& spans = segctx.GetSegHandler()->GetListItemSpans();
            for(TTypedSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
                st.InsertSpan(*it, Sprintf("ListItem%lu", it->Depth));
            }
        }

        {
            const TTypedSpans& spans = segctx.GetSegHandler()->GetListSpans();
            for(TTypedSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
                st.InsertSpan(*it, Sprintf("List%lu", it->Depth));
            }
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
    TStructZonesTest test;
    test.Init(argc, argv);
    test.RunTest();
}
