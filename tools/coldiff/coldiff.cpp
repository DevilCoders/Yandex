#include <library/cpp/string_utils/col_diff/coldiff.h>
#include <library/cpp/deprecated/split/delim_string_iter.h>
#include <library/cpp/deprecated/prog_options/prog_options.h>

#include <util/generic/strbuf.h>

#include <util/generic/buffer.h>
#include <util/generic/yexception.h>

#include <util/stream/file.h>
#include <util/stream/output.h>

#include <util/string/cast.h>

class TReader
{
    size_t    NKeyFields;

    TString    Filename;
    TIFStream In;
    bool      More;
    TString    Line;
    ssize_t   NLine;

    TStringBuf Key;
    TStringBuf Value;
public:
    TReader(const TString& filename, size_t nKeyFields)
        : NKeyFields(nKeyFields)
        , Filename(filename)
        , In(filename)
    {
        NLine = -1;
        Next();
    }

    bool Next()
    {
        TString oldKey;
        if (NLine != -1) {
            oldKey = Key;
        }
        More = In.ReadLine(Line);
        if (More) {
            ++NLine;

            TDelimStringIter it(Line, "\t");
            if (NKeyFields > 0) {
                it += (NKeyFields-1);
                if (!it.Valid())
                    ythrow yexception() << "Invalid key in file " << Filename << " at line " << NLine;
                Key = TStringBuf(Line.begin(), it.GetEnd());
                if (NLine != 0) {
                    if (TStringBuf::compare(Key,oldKey) <= 0) {
                        ythrow yexception() << "Nonincreasing key in file " << Filename << " at line " << NLine;
                    }
                }
                ++it;
            }
            Value = TStringBuf(it.GetBegin(), Line.end());
        }
        return More;
    }

    bool Eof() const
    {
        return !More;
    }

    TString GetLine() const
    {
        return Line;
    }

    TStringBuf GetKey() const
    {
        return Key;
    }
    TStringBuf GetValue() const
    {
        return Value;
    }

    size_t GetNLine() const
    {
        return (size_t)NLine;
    }
};


void Out(size_t ind, TReader& r)
{
    Cout << ind << ':' << r.GetNLine() << '\t' << r.GetLine() << Endl;
}

void OutRem(size_t ind, TReader& r)
{
    for ( ;!r.Eof(); r.Next()) {
        Out(ind, r);
    }
}

void InitSet(TOptRes opt, THashSet<size_t>& res)
{
    if (opt.first) {
        for (TDelimStringIter it(TStringBuf(opt.second), ","); it.Valid(); ++it) {
            res.insert(FromString<size_t>(*it));
        }
    }
}

void InitFilter(TProgramOptions& progOptions, THolder< TIncExcFilter<size_t> >& filter)
{
    THashSet<size_t> inc;
    InitSet(progOptions.GetOption("inc"), inc);
    THashSet<size_t> exc;
    InitSet(progOptions.GetOption("exc"), exc);
    filter.Reset(new TIncExcFilter<size_t>(inc, exc));
}


int ColDiffMain(TProgramOptions& progOptions)
{
    const TVector<const char*>& uno = progOptions.GetAllUnnamedOptions();
    if (uno.size() != 3) {
        throw TProgOptionsException("need 3 unnamed args");
    }

    size_t nKeyFields = FromString<size_t>(uno[0]);

    TReader r0(uno[1], nKeyFields);
    TReader r1(uno[2], nKeyFields);

    THolder< TIncExcFilter<size_t> > filter;
    InitFilter(progOptions, filter);

    TBufferOutput out;

    while (true) {
        if (r0.Eof()) {
            OutRem(1,r1);
            break;
        } else if (r1.Eof()) {
            OutRem(0,r0);
            break;
        } else {
            TStringBuf k0 = r0.GetKey();
            TStringBuf k1 = r1.GetKey();

            if (k0 < k1) {
                Out(0,r0);
                r0.Next();
            } else if (k1 < k0) {
                Out(1,r1);
                r1.Next();
            } else {
                ColDiff(r0.GetValue(), r1.GetValue(), *filter, out);
                TBuffer& outBuf = out.Buffer();
                if (!outBuf.Empty()) {
                    Cout << "01:" << r0.GetNLine() << ':' << r1.GetNLine()
                        << '\t' << k0 << '\t';

                    Cout.Write(outBuf.Data(), outBuf.Size());
                    Cout << Endl;
                    outBuf.Clear();
                }
                r0.Next();
                r1.Next();
            }
        }
    }

    return 0;
}

void PrintHelp()
{
    Cout << "Usage: coldiff [-inc f1[,f2..]] [-exc f1[,f2..]] <nKeyFields> <srcfile1> <srcfile2>\n"
            " first nKeyFields are simply printed, not compared\n";
}

int main(int argc, const char* argv[])
{
    TProgramOptions progOptions("|inc|+|exc|+");
    return main_with_options_and_catch(progOptions, argc, argv, ColDiffMain, PrintHelp);
}

