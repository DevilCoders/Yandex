#include <library/cpp/containers/dense_hash/dense_hash.h>
#include <library/cpp/getopt/last_getopt.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>

#include <library/cpp/diff/diff.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/stream/buffered.h>
#include <util/string/builder.h>
#include <util/string/split.h>

using namespace NYT;
using namespace NDiff;

const char* HRULE = "-----------------------------------------------------------------------";

class TDiffFound: public yexception {
public:
    using yexception::yexception;
};

template <class T, class S>
static inline void Split(const TBlob& b, T& v, S& s) {
    for (const auto& x : StringSplitter(b.AsCharPtr(), b.Size()).Split('\n')) {
        const TStringBuf t(x.Token().begin(), x.Delim().end());

        if (t) {
            const auto h = ComputeHash(t) << 1;

            v.push_back(h);
            s[h] = t;
        }
    }
}

template <class T>
static inline TConstArrayRef<T> ToArrayRef(const TVector<T>& v) {
    return {v.data(), v.size()};
}

static inline void DoOut(IOutputStream& out, const TStringBuf& s) {
    if (s && *(s.end() - 1) == '\n') {
        out << s;
    } else {
        out << s << "\n\\ No newline at end of file\n"sv;
    }
}

static int PrintDiff(const TBlob& f, const TBlob& t, IOutputStream& cout) {
    using TStr = ui64;

    TVector<TStr> fv;
    TVector<TStr> tv;

    TDenseHash<TStr, TStringBuf> ss;

    Split(f, fv, ss);
    tv.reserve(fv.size());
    Split(t, tv, ss);

    TVector<TChunk<TStr>> res;

    InlineDiff(res, ToArrayRef(fv), ToArrayRef(tv));

    ui64 c1 = 1;
    ui64 c2 = 1;

    int rc = 0;

    for (const auto& p : res) {
        if (p.Left || p.Right) {
            rc = 1;
            char mode;

            if (p.Left) {
                if (p.Right) {
                    mode = 'c';
                } else {
                    mode = 'd';
                }
            } else {
                mode = 'a';
            }

            cout << (c1 - (ui64)(mode == 'a' ? 1 : 0));

            if (mode != 'a') {
                const ui64 c1_n = c1 + p.Left.size() - (ui64)1;

                if (c1_n != c1) {
                    cout << ',' << c1_n;
                }
            }

            cout << mode << (c2 - (ui64)(mode == 'd' ? 1 : 0));

            if (mode != 'd') {
                const ui64 c2_n = c2 + p.Right.size() - (ui64)1;

                if (c2_n != c2) {
                    cout << ',' << c2_n;
                }
            }

            cout << '\n';

            for (const auto& l : p.Left) {
                DoOut(cout << "< "sv, ss.Value(l, TStringBuf{}));
            }

            if (mode == 'c') {
                cout << "---\n"sv;
            }

            for (const auto& r : p.Right) {
                DoOut(cout << "> "sv, ss.Value(r, TStringBuf{}));
            }
        }

        c1 += p.Common.size() + p.Left.size();
        c2 += p.Common.size() + p.Right.size();
    }

    return rc;
}

static int PrintDiff(const TString& lhs, const TString& rhs, IOutputStream& out) {
    return PrintDiff(TBlob::FromString(lhs), TBlob::FromString(rhs), out);
}

namespace {

    class TDiffReducer
       : public IReducer<TTableReader<TNode>, TTableWriter<TNode>> {

    public:
        void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
            TNode left = TNode::CreateMap();
            TNode right = TNode::CreateMap();
            size_t count = 0;
            for (; input->IsValid(); input->Next()) {
                const TNode& row = input->GetRow();
                if (count > 1) {
                    ++count;
                    continue;
                }
                if (count) {
                    right = row;
                } else {
                    left = row;
                }
                ++count;
            }
            if (left.HasKey("data") && right.HasKey("data")) {
                if (left["data"].AsString() != right["data"].AsString()) {
                    TStringStream diff;
                    PrintDiff(left["data"].AsString(), right["data"].AsString(), diff);
                    TNode diffRow = left;
                    diffRow["diff"] = diff.Str();
                    diffRow["right_data"] = right["data"].AsString();
                    output->AddRow(diffRow);
                }
            }
        }

        TDiffReducer()
        {
        }
    };

    REGISTER_REDUCER(TDiffReducer);
}

int main(int argc, char *argv[]) {
    NYT::Initialize(argc, (const char **)argv);
    NYT::SetLogger(NYT::CreateStdErrLogger(ILogger::INFO));

    NLastGetopt::TOpts options;

    TString leftLogTable;
    options
        .AddLongOption('l', "left_path", "Path to input log table")
        .Required()
        .RequiredArgument("INPUT")
        .StoreResult(&leftLogTable);

    TString rightLogTable;
    options
        .AddLongOption('r', "right_path", "Path to input log table")
        .Required()
        .RequiredArgument("INPUT")
        .StoreResult(&rightLogTable);

    TString outputPath;
    options
        .AddLongOption('o', "output_table", "Path to output table")
        .Required()
        .RequiredArgument("OUTPUT")
        .StoreResult(&outputPath);

    TMaybe<TString> pool;
    options
        .AddLongOption('p', "pool", "YT pool")
        .Optional()
        .StoreResultT<TString>(&pool);

    TString proxy;
    options.
        AddLongOption('x', "proxy", "YT proxy")
        .Optional()
        .DefaultValue("freud")
        .StoreResult(&proxy);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    TNode commonSpec = TNode()("title", "run-diff");
    if (pool.Defined()) {
        commonSpec("pool", *pool);
    }

    auto client = NYT::CreateClient(proxy);

    client->Sort(
        TSortOperationSpec()
            .AddInput(TRichYPath(leftLogTable))
            .Output(TRichYPath(leftLogTable))
            .SortBy({"instance", "frame_id", "ts"}),
        TOperationOptions().Spec(commonSpec)
    );

    client->Sort(
        TSortOperationSpec()
            .AddInput(TRichYPath(rightLogTable))
            .Output(TRichYPath(rightLogTable))
            .SortBy({"instance", "frame_id", "ts"}),
        TOperationOptions().Spec(commonSpec)
    );

    client->Reduce(
        TReduceOperationSpec()
            .ReduceBy({"instance", "frame_id", "ts"})
            .SortBy({"instance", "frame_id", "ts"})
            .AddInput<TNode>(TRichYPath(leftLogTable).SortedBy({"instance", "frame_id", "ts"}))
            .AddInput<TNode>(TRichYPath(rightLogTable).SortedBy({"instance", "frame_id", "ts"}))
            .AddOutput<TNode>(TRichYPath(outputPath + TString(".diff")))
            .ReducerSpec(TUserJobSpec()),
        new TDiffReducer(),
        TOperationOptions().Spec(commonSpec));

    return 0;
}
