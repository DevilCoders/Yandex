#include <tools/snipmake/contentstats/util/kiwi.h>
#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/stream/buffered.h>

int main(int argc, const char* argv[])
{
    Y_UNUSED(argc);
    Y_UNUSED(argv);

    NHtmlStats::TParserOptions options;
    //options.DoReplaceNumbers();
    NHtmlStats::TLearner learner("mined_groups.txt");
    NHtmlStats::TProtoStreamIterator iter(&Cin);

    int recnum = 0;
    NHtmlStats::TProtoDoc doc;
    while (iter.Next(doc)) {
        ++recnum;

        if (!doc.Html || !doc.Url || doc.Encoding < 0 || doc.Encoding >= CODES_MAX) {
            TString id = doc.Url;
            if (!id) {
                id = ToString<int>(recnum);
            }
            Cerr << "Record " << id << " is incomplete (needs non-empty URL & Html and valid Encoding)" << Endl;
            continue;
        }

        learner.StudyDocument(doc.Url, doc.Html, doc.Encoding, options);
    }

    TBufferedOutput out(&Cout);
    learner.WriteToStream(out);
    out.Finish();
}

