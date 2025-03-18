#include <library/cpp/containers/dense_hash/dense_hash.h>
#include <library/cpp/diff/diff.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/string/split.h>
#include <util/stream/buffered.h>
#include <util/digest/murmur.h>

using namespace NDiff;

template <class T, class S>
inline void Split(const TBlob& b, T& v, S& s, ui64 sl) {
    for (const auto& x : StringSplitter(b.AsCharPtr(), b.Size()).Split('\n')) {
        if (sl && sl--) {
            v.push_back(0);
            continue;
        }

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

int main(int argc, char** argv) {
    NLastGetopt::TOpts opts;

    ui64 sl = 0;

    opts.SetFreeArgsMin(2);
    opts.SetFreeArgsMax(2);
    opts.SetFreeArgTitle(0, "FILE", "path to the file 1");
    opts.SetFreeArgTitle(1, "FILE", "path to the file 2");
    opts.AddCharOption('l', "skip first l lines").DefaultValue(0).StoreResult(&sl);
    opts.AddHelpOption('?');

    if (argc <= 1) {
        opts.PrintUsage(argv[0] ? argv[0] : "fast_diff");
        exit(1);
    }

    NLastGetopt::TOptsParseResult optr(&opts, argc, argv);
    TVector<TString> files = optr.GetFreeArgs();

    const TBlob f = TBlob::FromFile(files[0]);
    const TBlob t = TBlob::FromFile(files[1]);

    using TStr = ui64;

    TVector<TStr> fv;
    TVector<TStr> tv;

    TDenseHash<TStr, TStringBuf> ss;

    Split(f, fv, ss, sl);
    tv.reserve(fv.size());
    Split(t, tv, ss, sl);

    TVector<TChunk<TStr>> res;

    InlineDiff(res, ToArrayRef(fv), ToArrayRef(tv));

    ui64 c1 = 1;
    ui64 c2 = 1;

    TAdaptiveBufferedOutput cout(&Cout);

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
