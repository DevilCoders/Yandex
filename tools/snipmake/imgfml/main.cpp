#include <util/stream/input.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/generic/strbuf.h>
#include <utility>

/*
 * usage is like this
 * cat ??/imgpool.txt | imgfml step1 | imgfml > ysite/yandex/snippets/formulae/imgboostfml.inc
 *
 * step1 is mostly written by steiner@
 *
 * where impool.txt is output from tools/snipmake/imglrn --second
 */

namespace NPassOne {

TVector<TString> Split(const TString& s, char ch = '\t') {
    TVector<TString> res;
    TCharDelimiter<const char> d(ch);
    TContainerConsumer< TVector<TString> > c(&res);
    SplitString(s.data(), s.data() + s.size(), d, c);
    return res;
}


int main () {
    TString s;
    typedef TVector<TString> TFactors;
    typedef TMap<double, int> TMarks;
    typedef TMap<TFactors, TMarks> TRatings;
    typedef TMap<TFactors, int> TRatingsSize;
    typedef TMap<double, int> TDistribution;
    TRatings a;
    TRatingsSize b;
    TDistribution distribution;
    int total = 0;

    while (Cin.ReadLine(s)) {
        TVector<TString> rec = Split(s);
        if (rec.size() < 4) {
            continue;
        }
        TVector<TString> allFact = Split(rec[1], ' ');
        TVector<TString> fact;
        for (size_t i = 0; i < allFact.size(); ++i)
            if (allFact[i].find("mf_img_") != TString::npos) {
                int pos = allFact[i].find(":");
                fact.push_back(allFact[i].substr(pos + 1));
            }
        double mark = FromString<double>(rec[2]);
        ++a[fact][mark];
        ++b[fact];
        ++distribution[mark];
        ++total;
    }
    for (TRatings::const_iterator it = a.begin(); it != a.end(); ++it) {
        for (size_t i = 0; i < it->first.size(); ++i) {
            Cout << it->first[i] << "\t";
        }
        double res = 0;
        for (TMarks::const_iterator it1 = it->second.begin(); it1 != it->second.end(); ++it1) {
            res += it1->first * it1->second;
        }
        res /= b[it->first];
        Cout << res << "\t" << b[it->first] << Endl;
    }
    double res = 0;
    for (TDistribution::iterator it = distribution.begin(); it != distribution.end(); ++it) {
        res += it->first * it->second;
    }
    res /= total;
    Cout << res << Endl;
    return 0;
}

}

namespace NPassTwo {

typedef TVector<int> TFactors;
typedef std::pair<TFactors, double> TPoint;
typedef TVector<TPoint> TPoints;
double avg = 0;

void Indent (int indent) {
    for (int i = 0; i < indent; ++i) {
        putchar ('\t');
    }
}

/* yeah, hard-coded for now */
const char* names[23] = {
"IsPornoAdv",
"IsYellowAdv",
"IsPopunder",

"IsClickunder",
"TrashAdvGE8",
"NoSize",

"Size",
"SiteMatch",
"SeriesMatch",

"NewsIfPolitician",

"DefHide",
"DefTic",
"NotDefForeign",

"UnaskedPorno",
"ComEng",
"DefRUk",

"TrDomTr",
"MainLangMatch",
"SecondLangMatch",

"ConstrPass",

"ExactWallpaper",
"LargerWallpaper",
"PornoPess",
};
bool goodf[23];

void out (TPoints::const_iterator b, TPoints::const_iterator e, int shift = 0, int indent = 1) {
    if (b == e) {
        throw 1;
    }
    if (b + 1 == e) {
        Indent (indent);
        Cout << "return " << b->second << ";" << Endl;
        return;
    }
    if (!goodf[shift]) {
        return out (b, e, shift + 1, indent);
    }

    for (TPoints::const_iterator i = b; i != e; ) {
        int x = i->first[shift];
        TPoints::const_iterator j = i + 1;
        while (j < e && j->first[shift] == x) {
            ++j;
        }
        Indent (indent);
        if (i > b) {
            Cout << "else ";
        }
        Cout << "if (f." << names[shift] << " == " << x << ") {" << Endl;
        out (i, j, shift + 1, indent + 1);
        Indent (indent);
        Cout << "}" << Endl;
        i = j;
    }
}

int main () {
    TString s;
    TPoints vs;
    while (Cin.ReadLine(s)) {
        if (s.find ('\t') == TString::npos) {
            avg = FromString<double>(s);
            break;
        }
        TVector<int> tmp;
        tmp.push_back (0);
        for (int q = 0; q < (int)s.size(); ++q) {
            if (s[q] == '\t') {
                tmp.push_back (q + 1);
            }
        }
        TPoint v;
        v.second = FromString<double>(TStringBuf(s.data() + tmp[tmp.size() - 2], s.data() + tmp.back() - 1));
        for (int i = 0; i < (int)tmp.size() - 2; ++i) {
            int f;
            f = FromString<int>(TStringBuf(s.data() + tmp[i], s.data() + tmp[i + 1] - 1));
            v.first.push_back (f);
        }
        vs.push_back (v);
    }
    for (int j = 0; j < 23; ++j) {
        for (int i = 1; i < (int)vs.size(); ++i) {
            if (vs[i].first[j] != vs[i - 1].first[j]) {
                goodf[j] = true;
                break;
            }
        }
    }
    Sort (vs.begin(), vs.end ());
    out (vs.begin (), vs.end ());
    Indent (1);
    Cout << "return " << avg << ";" << Endl;
    return 0;
}

}

int main (int argc, char **argv) {
    --argc, ++argv;
    if (argc > 0) {
        return NPassOne::main();
    } else {
        return NPassTwo::main();
    }
}
