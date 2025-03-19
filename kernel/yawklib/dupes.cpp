#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <library/cpp/containers/rarefied_array/rarefied_array.h>
#include <library/cpp/containers/ext_priority_queue/ext_priority_queue.h>
#include <library/cpp/containers/2d_array/2d_array.h>
#include "wtrutil.h"
#include "dupes.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class TStr>
int LevenshteinDistImpl(const TStr &w1, const TStr &w2)
{
    size_t m = w1.size(), n = w2.size();
    TArray2D<int> d(n + 1, m + 1);
    d[0][0] = 0;
    for (size_t i = 1; i <= m; ++i)
        d[i][0] = i;
    for (size_t j = 1; j <= n; ++j)
        d[0][j] = j;

    for (size_t j = 1; j <= n; ++j) {
        for (size_t i = 1; i <= m; ++i) {
            if (w1[i-1] == w2[j-1])
                d[i][j] = d[i-1][j-1];
            else {
                int dist1 = d[i-1][j] + 1;
                int dist2 = d[i][j-1] + 1;
                int dist3 = d[i-1][j-1] + 1;
                d[i][j] = Min(Min(dist1, dist2), dist3);
            }
        }
    }

    return d[m][n];
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int LevenshteinDist(const TStringBuf &w1, const TStringBuf &w2)
{
    return LevenshteinDistImpl(w1, w2);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int LevenshteinDist(const TWtringBuf &w1, const TWtringBuf &w2)
{
    return LevenshteinDistImpl(w1, w2);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static const char* TheDupesVariants[] = {
    "а|a",
    "б|b",
    "в|v",
    "г|g",
    "д|d",
    "е|e|ye|je",
    "же|je",
    "дже|je",
    "ё|e", // latinic e <=> cyrillic ё
    "ё|йо|yo|jo|io",
    "жo|jo",
    "джo|jo|joe",
    "ио|io",
    "ьо|io",
    "е|ё", // cyrillic е <=> cyrillic ё
    "ж|zh|g|j",
    "дж|g|j",
    "з|z|s",
    "и|i|y|e",
    "й|i|j|y",
    "к|k|c|q",
    "л|l",
    "м|m",
    "н|n",
    "о|o",
    "п|p",
    "р|r",
    "с|s", // cyrillic c <=> latin s
    "с|c", // cyrillic c <=> latin c
    "т|t",
    "у|u|w",
    "ф|f|ff|фф",
    "ф|f|ph"
    "х|h|kh|ch",
    "ц|ts|c|z",
    "ч|ch|c|tch",
    "ш|sh|sch",
    "щ|sh|tsh|ch|sch",
    "ъ|'|",
    "ы|i|y",
    "ь|'|",
    "э|a|e", // latin a and e
    "э|е", // cyrillic letter е
    "ю|yu|ju|у",
    "жу|ju",
    "джу|ju",
    "я|ya|ja",
    "жа|ja",
    "джа|ja",
    "жи|ji",
    "джи|ji",
    "б|бб",
    "в|вв",
    "г|гг",
    "д|дд",
    "ж|жж",
    "з|зз",
    "к|кк",
    "л|лл",
    "м|мм",
    "н|нн",
    "п|пп",
    "с|сс",
    "р|рр",
    "т|тт",
    "ф|фф",
    "х|хх",
    "ц|цц",
    "ч|чч",
    "ш|шш",
    "щ|щщ",
    "qu|к|ку|кв",
    "qui|куа|кви|куи",
    "que|к",
    "ou|у|ау|оу",
    "th|т",
    "th|ф",
    "th|с|з",
    "ng|н|нг|нь",
    "su|шу|жу",
    "x|х", // cyrillic <-> latin letters x
    "x|ks|кс", // latin letter x
    " икс | x ",

    " the |",
    " a |",
    " and | и |+| +",
    "+| +| плюс | plus ",
    " в | ",

    "0|ноль",
    "1|один|one|first|первый|1st|1й|1 st|1 й|i",
    "2|два|two|второй|second|2nd|2й|2nd|2 й|ii",
    "3|три|three|третий|third|3rd|3й|3 rd|3 й|iii",
    "4|четыре|four|четвертый|fourth|4th|4й|4 th|4 й|iv",
    "5|пять|five|пятый|fifth|5th|5й|5 th|5 й|v",
    "6|шесть|six|шестой|sixth|6th|6й|6 th|6 й|vi",
    "7|семь|seven|седьмой|seventh|7th|7й|7 th|7 й|vii",
    "8|восемь|eight|восьмой|8th|8й|8 th|8 й|viii",
    "9|девять|nine|девятый|9th|9й|9 th|9 й|ix",
    "10|десять|ten|десятый|10th|10й|10 th|10 й",
    "11|одиннадцать|eleven|11th|11й|11 th|11 й|xi",
    "12|двенадцать|twelve|12th|12й|12 th|12 й|xii",
    "13|тринадцать|thirteen|13th|13й|13 th|13 й|xiii",

    "20|двадцать|twenty|20th|20й|20 th|20 й|xx",
    "21|двадцать один|twenty one|двадцать первый|21st|21й|21 th|21 й|xxi",
    "1001|тысяча и один|тысяча и одна",

    "moscow |москва |москвы |в москве ",
    "спб |санкт петербург |петербург ",
    "season |сезон ",
    "restaurant |ресторан ",
    "авто|auto",
    "film |фильм ",
    " имени | им ",
    " улица | ул ",
    nullptr
};
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool StartsWith(const TUtf16String &w1, size_t start, const TUtf16String &w2)
{
    for (size_t n = 0; n < w2.size(); ++n) {
        if (w1[n + start] != w2[n])
            return false;
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class TDuplicatesChecker
{
    THashMap<TUtf16String, TVector<TUtf16String> > Variants;
    TVector<TUtf16String> Skipable;
    size_t MaxFragment;

    static void Set(bool& b, bool val) {
        b = val;
    }

    static void Set(size_t& n, size_t val) {
        if (!n || n > val)
            n = val;
    }

    template<class T>
    void MoveOn(const TUtf16String &phrase1, const TUtf16String &phrase2, size_t start1, size_t start2, TArray2D<T> &res, T val) const {
        if (phrase1[start1] == ' ')
            Set(res[start1 + 1][start2], val);
        if (phrase2[start2] == ' ')
            Set(res[start1][start2 + 1], val);

        size_t len = 0;
        size_t until = Min(phrase1.size() - start1, phrase2.size() - start2);
        while (len < until && phrase1[start1 + len] == phrase2[start2 + len]) {
            ++len;
            Set(res[start1 + len][start2 + len], val);
        }

        for (size_t n = 0; n < Skipable.size(); ++n)
            if (StartsWith(phrase2, start2, Skipable[n]))
                Set(res[start1][start2 + Skipable[n].size()], val);

        size_t maxFragment = Min(MaxFragment, phrase1.size() - start1);
        TUtf16String beg;
        for (len = 1; len <= maxFragment; ++len) {
            beg.push_back(phrase1[start1 + len - 1]);
            THashMap<TUtf16String, TVector<TUtf16String> >::const_iterator it = Variants.find(beg);
            if (it == Variants.end())
                continue;
            for (size_t n = 0; n < it->second.size(); ++n) {
                if (StartsWith(phrase2, start2, it->second[n])) {
                    size_t len2 = it->second[n].size();
                    Set(res[start1 + len][start2 + len2], val);
                }
            }
        }
    }
public:
    TDuplicatesChecker(const char **dupesList) {
        MaxFragment = 0;
        for (const char **it = dupesList; *it; ++it) {
            TString next = *it;
            TVector<const char*> variants;
            Split(next.begin(), '|', &variants);
            for (size_t n = 0; n < variants.size(); ++n) {
                TVector<TUtf16String> &dst = Variants[UTF8ToWide(variants[n])];
                for (size_t m = 0; m < variants.size(); ++m) {
                    if (m == n)
                        continue;
                    TUtf16String w = UTF8ToWide(variants[m]);
                    if (MaxFragment < w.size())
                        MaxFragment = w.size();
                    if (!Contains(dst, w))
                        dst.push_back(w);
                }
            }
        }
        Skipable = Variants[TUtf16String()];
    }

    bool IsDuplicates(const TUtf16String &phrase1, const TUtf16String &phrase2) const {
        size_t m = phrase1.size(), n = phrase2.size();
        TArray2D<bool> d(n + 1, m + 1);
        d.FillZero();
        d[0][0] = true;
        for (size_t x = 0; x < m; ++x) {
            for (size_t y = 0; y < n; ++y) {
                if (d[x][y]) {
                    MoveOn(phrase1, phrase2, x, y, d, true);
                    if (d[m][n])
                        return true;
                }
            }
        }
        return false;
    }

    size_t FindDuplicate(const TUtf16String &phrase, const TUtf16String &substr, size_t start, size_t *tail) const {
        if (tail)
            *tail = TUtf16String::npos;
        if (start >= phrase.size())
            return TUtf16String::npos;
        size_t m = phrase.size(), n = substr.size();
        TArray2D<size_t> d(n + 1, m + 1);
        d.FillZero();
        for (size_t x = start; x <= m; ++x)
            d[x][0] = x + 1;
        for (size_t x = start; x < m; ++x) {
            for (size_t y = 0; y < n; ++y)
                if (d[x][y])
                    MoveOn(phrase, substr, x, y, d, d[x][y]);
            for (size_t t = x + 1; t <= m; ++t) {
                if (d[t][n]) {
                    if (tail)
                        *tail = t;
                    return d[t][n] - 1;
                }
            }
        }
        return TUtf16String::npos;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class TOrdinaryDuplicatesChecker: public TDuplicatesChecker
{
public:
    TOrdinaryDuplicatesChecker()
        : TDuplicatesChecker(TheDupesVariants) {
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsDuplicates(const TUtf16String &phrase1, const TUtf16String &phrase2)
{
    TDuplicatesChecker* duplicatesChecker = Singleton<TOrdinaryDuplicatesChecker>();
    return duplicatesChecker->IsDuplicates(phrase1, phrase2);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
size_t FindDuplicate(const TUtf16String &phrase, const TUtf16String &substr, size_t start, size_t *tail)
{
    TDuplicatesChecker* duplicatesChecker = Singleton<TOrdinaryDuplicatesChecker>();
    return duplicatesChecker->FindDuplicate(phrase, substr, start, tail);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/*struct TTestFindDupe {
    TTestFindDupe() {
        bool b1 = FindDuplicate(u"1111", u"222") == TUtf16String::npos;
        bool b2 = FindDuplicate(u"1111", u"111") == 0;
        bool b3 = FindDuplicate(u"1111", u"111", 1) == 1;
        bool b4 = FindDuplicate(u"1111", u"1111") == 0;
        bool b5 = FindDuplicate(u"1111", u"11111") == TUtf16String::npos;
        bool b6 = FindDuplicate(u"три мушкетера", u"3мушкетера") != TUtf16String::npos;
        bool b7 = FindDuplicate(u"три мушкетера и какой-то хрен из Гаскони", u"3мушкетера") != TUtf16String::npos;
        size_t checkEmpty = FindDuplicate(u"три мушкетера", TUtf16String());
        size_t checkEmpty2 = FindDuplicate(TUtf16String(), TUtf16String());
        size_t checkPos = FindDuplicate(u"один три", u"3");
        size_t checkPos2 = FindDuplicate(u"1 3 3", u"три три");
        size_t oneMoreCheck = FindDuplicate(u"косметика клиник", u"clinique");
    }
} testFindDupe;*/
////////////////////////////////////////////////////////////////////////////////////////////////////
class TShingles
{
    typedef THashMap<TUtf16String, TVector<int> > TEvidence;
    TEvidence Evidences;

    TRarefiedArray<bool> KnownDupes;

    const TVector<TUtf16String> &Src;
    TDupes *Res;

    int ComplexCallsProfilingCounter;

protected:
    virtual bool AreRealDupes(const TUtf16String &w1, const TUtf16String &w2) const = 0;
    virtual ~TShingles() {
    }
public:
    TShingles(const TVector<TUtf16String> &src, TDupes *res)
        : Src(src)
        , Res(res)
        , ComplexCallsProfilingCounter(0)
    {
        KnownDupes.Init(src.size());
    }
    void Next() {
        KnownDupes.clear();
    }
    void AddStrongEvidence(const TUtf16String &sh, int id) {
        TEvidence::iterator it = Evidences.find(sh);
        if (it != Evidences.end()) {
            for (size_t n = 0; n < it->second.size(); ++n)
                if (it->second[n] == id)
                    return;
            for (size_t n = 0; n < it->second.size(); ++n) {
                int dupeId = it->second[n];
                bool &known = KnownDupes[dupeId];
                if (!known) {
                    ++ComplexCallsProfilingCounter;
                    if (AreRealDupes(Src[id], Src[dupeId]))
                        Res->push_back(std::make_pair(dupeId, id));
                }
                known = true;
            }
            it->second.push_back(id);
        }
        else
            Evidences[sh].push_back(id);
    }
    int GetProfilingCounter() const {
        return ComplexCallsProfilingCounter;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class TLevenshteinShingles: public TShingles
{
public:
    TLevenshteinShingles(const TVector<TUtf16String> &src, TDupes *res)
        : TShingles(src, res)
    {
    }
    bool AreRealDupes(const TUtf16String &w1, const TUtf16String &w2) const override {
        return LevenshteinDist(w1, w2) <= (int)Min(w1.size(), w2.size()) / 4;
    }
};
static void AddOneCharOuts(const TUtf16String &w, int id, TShingles &res)
{
    for (size_t n = 0; n < w.size(); ++n)
        res.AddStrongEvidence(w.substr(0, n) + w.substr(n+1), id);
}
static void AddTwoSequentialCharOuts(const TUtf16String &w, int id, TShingles &res)
{
    for (size_t n = 0; n < w.size() - 1; ++n)
        res.AddStrongEvidence(w.substr(0, n) + w.substr(n+2), id);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void SelectLevenshteinDupes(const TVector<TUtf16String> &src, TDupes *res)
{
    TLevenshteinShingles shingles(src, res);
    for (size_t n = 0; n < src.size(); ++n, shingles.Next()) {
        TUtf16String word = src[n];
        if (word.size() <= 3)
            continue;
        shingles.AddStrongEvidence(word, n);
        AddOneCharOuts(word, n, shingles);
        if (word.size() >= 5)
            AddTwoSequentialCharOuts(word, n, shingles);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class TLevenshteinShinglesForLongPhrases: public TShingles
{
    float MaxDist;
public:
    TLevenshteinShinglesForLongPhrases(const TVector<TUtf16String> &src, TDupes *res, float maxDist)
        : TShingles(src, res)
        , MaxDist(maxDist)
    {
    }
    bool AreRealDupes(const TUtf16String &w1, const TUtf16String &w2) const override {
        return LevenshteinDist(w1, w2) <= (int)Max(w1.size(), w2.size()) * MaxDist;
    }
};
void SelectLevenshteinDupesForLongPhrases(const TVector<TUtf16String> &src, float maxDistanceRateForDuplicate, int shingleSize, TDupes *res) {
    TLevenshteinShinglesForLongPhrases shingles(src, res, maxDistanceRateForDuplicate);
    for (size_t n = 0; n < src.size(); ++n, shingles.Next()) {
        TUtf16String word = src[n];
        for (size_t m = 0; m + shingleSize <= word.size(); ++m)
            shingles.AddStrongEvidence(word.substr(m, m + shingleSize), n);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class TGreedyDecoder // todo: replace it's implementation with trie for additional speed
{
    THashMap<TUtf16String, TUtf16String> Data;
    size_t LargestKnown;
    struct TSizeDescr {
        int NumKnown;
        TUtf16String Example;
        TUtf16String Result;
        TSizeDescr()
            : NumKnown(0)
        {
        }
    };
    TVector<TSizeDescr> BySize;
public:
    void Init(const THashMap<TUtf16String, TUtf16String> &data)
    {
        Data = data;
        LargestKnown = 0;
        for (THashMap<TUtf16String, TUtf16String>::const_iterator it = data.begin(); it != data.end(); ++it) {
            size_t size = it->first.size();
            if (size == 0)
                continue;
            if (size >= BySize.size())
                BySize.resize(size + 1);
            TSizeDescr &descr = BySize[size];
            ++descr.NumKnown;
            if (descr.NumKnown == 1) {
                descr.Example = it->first;
                descr.Result = it->second;
            }
        }
    }
    size_t FindMatch(const TUtf16String &src, size_t from, TUtf16String &test, TUtf16String &res) const {
        size_t startSize = test.size();
        for (size_t len = startSize; len > 0; --len) {
            const TSizeDescr &descr = BySize[len];
            if (descr.NumKnown == 1) {
                if (test == descr.Example) {
                    res += descr.Result;
                    return len;
                }
            } else if (descr.NumKnown > 0) {
                THashMap<TUtf16String, TUtf16String>::const_iterator it = Data.find(test);
                if (it != Data.end()) {
                    res += it->second;
                    return len;
                }
            }
            test.pop_back();
        }
        res += src[from];
        return 1;
    }
    //void FindLargestMatch(const TUtf16String &src, size_t &from, TUtf16String &res) const {
    //    Y_ASSERT(from < +src);
    //    size_t startSize = Min(+BySize - 1, +src - from);
    //    TUtf16String test = src.substr(from, startSize);
    //    from += FindMatch(src, from, test, res);
    //}
    void FindMatches(const TUtf16String &src, size_t from, const TUtf16String &evidence, TExtPriorityQueue<TUtf16String> *res) const {
        Y_ASSERT(from < src.size());
        TUtf16String add;
        size_t len = Min(BySize.size() - 1, src.size() - from);
        TUtf16String test = src.substr(from, len);
        while (add.empty() && !test.empty()) {
            len = FindMatch(src, from, test, add);
            res->push(evidence + add, from + len);
            if (!test.empty())
                test.pop_back();
        }
    }
};

class TDuplicatesShingles: public TShingles
{
    TGreedyDecoder Decoder;
    TDuplicatesChecker DuplicatesChecker;
public:
    TDuplicatesShingles(const TVector<TUtf16String> &src0, TDupes *res, const char **dupesVariants)
        : TShingles(src0, res)
        , DuplicatesChecker(dupesVariants)
    {
        THashMap<TUtf16String, int> wordClasses;
        TVector<TVector<TUtf16String> > classWords;
        int curClass = 0;

        for (const char **it = dupesVariants; *it; ++it, ++curClass) {
            classWords.emplace_back();
            int mergeTo = curClass;
            TString next = *it;
            TVector<const char*> variants;
            Split(next.begin(), '|', &variants);
            for (size_t n1 = 0; n1 < variants.size(); ++n1) {
                TUtf16String variant = UTF8ToWide(variants[n1]);
                THashMap<TUtf16String, int>::iterator knownIt = wordClasses.find(variant);
                if (knownIt == wordClasses.end()) {
                    classWords[mergeTo].push_back(variant);
                    wordClasses[variant] = mergeTo;
                } else if (knownIt->second != mergeTo) {
                    int mergeFrom = knownIt->second;
                    if (mergeTo > knownIt->second) {
                        mergeFrom = mergeTo;
                        mergeTo = knownIt->second;
                    }
                    TVector<TUtf16String> &src = classWords[mergeFrom];
                    TVector<TUtf16String> &dst = classWords[mergeTo];
                    size_t to = dst.size();
                    dst.resize(to + src.size());
                    for (size_t n2 = 0; n2 < src.size(); ++n2, ++to) {
                        dst[to] = src[n2];
                        wordClasses[src[n2]] = mergeTo;
                    }
                    src.clear();
                }
            }
        }
        THashMap<TUtf16String, TUtf16String> data;
        for (size_t n = 0; n < classWords.size(); ++n) {
            if (classWords[n].empty())
                continue;
            TUtf16String smallest = classWords[n][0];
            for (size_t m = 1; m < classWords[n].size(); ++m)
                if (smallest.size() > classWords[n][m].size())
                    smallest = classWords[n][m];
            for (size_t m = 0; m < classWords[n].size(); ++m)
                data[classWords[n][m]] = smallest;
        }
        data[u" "] = TUtf16String();
        Decoder.Init(data);
    }
    bool AreRealDupes(const TUtf16String &w1, const TUtf16String &w2) const override {
        TUtf16String check1 = u" " + w1 + ' ';
        TUtf16String check2 = u" " + w2 + ' ';
        return DuplicatesChecker.IsDuplicates(check1, check2);
    }
    void AddAllEvidences(TUtf16String phrase, int id) {
        phrase = u" " + phrase + ' ';
        TExtPriorityQueue<TUtf16String> recognQueue;
        recognQueue.push(TUtf16String(), 0);
        while (!recognQueue.empty()) {
            TUtf16String evidence = recognQueue.ytop();
            size_t from = recognQueue.top().Pri;
            if (from >= phrase.size())
                AddStrongEvidence(evidence, id);
            else
                Decoder.FindMatches(phrase, from, evidence, &recognQueue);
            if (recognQueue.size() > 10000)   // in case of very exotic or strange queries
                break;
            recognQueue.pop();
        }
    }
};

void SelectDuplicates(const TVector<TUtf16String> &src, TDupes *res, const char **dupesVariants, bool printProfilingCounter)
{
    TDuplicatesShingles shingles(src, res, dupesVariants? dupesVariants : TheDupesVariants);
    for (size_t n = 0; n < src.size(); ++n, shingles.Next()) {
        if (src[n].size() <= 3)
            continue;
        shingles.AddAllEvidences(src[n], n);
    }
    if (printProfilingCounter)
        printf("Total comparisons: %d\n", shingles.GetProfilingCounter());
}

void PairsToClusters(const TDupes &src0, TVector<TVector<int> > *resClusters)
{
    Y_ASSERT(resClusters->empty());
    if (src0.empty())
        return;
    int maxId = Max(src0[0].first, src0[0].second);
    for (size_t n = 1; n < src0.size(); ++n) {
        if (src0[n].first > maxId)
            maxId = src0[n].first;
        if (src0[n].second > maxId)
            maxId = src0[n].second;
    }
    TVector<int> idToCluster(maxId + 1, -1);
    TVector<TVector<int> > clusters(src0.size());
    for (size_t cluster = 0; cluster < src0.size(); ++cluster) {
        int id1 = src0[cluster].first;
        int id2 = src0[cluster].second;
        int cl1 = idToCluster[id1];
        int cl2 = idToCluster[id2];
        if (cl1 < 0 && cl2 < 0) {
            idToCluster[id1] = cluster;
            clusters[cluster].push_back(id1);
            idToCluster[id2] = cluster;
            clusters[cluster].push_back(id2);
            continue;
        } else if (cl1 < 0 /* && cl2 >= 0 */) {
            idToCluster[id1] = cl2;
            clusters[cl2].push_back(id1);
        } else if (cl2 < 0 /* && cl1 >= 0 */) {
            idToCluster[id2] = cl1;
            clusters[cl1].push_back(id2);
        } else if (cl1 != cl2 /* && cl1 != 0 && cl2 != 0*/) {
            int mergeFrom = Max(cl1, cl2), mergeTo = Min(cl1, cl2);
            TVector<int> &src1 = clusters[mergeFrom];
            TVector<int> &dst = clusters[mergeTo];
            size_t to = dst.size();
            dst.resize(to + src1.size());
            for (size_t n = 0; n < src1.size(); ++n, ++to) {
                dst[to] = src1[n];
                idToCluster[src1[n]] = mergeTo;
            }
            src1.clear();
        }
    }

    resClusters->reserve(clusters.size());
    for (size_t n = 0; n < clusters.size(); ++n) {
        if (clusters[n].size()) {
            resClusters->push_back(std::move(clusters[n]));
        }
    }
}

void AddSingleElementClusters(size_t numElements, TClusters *res)
{
    TClusters &clusters = *res;
    TVector<int> done(numElements);
    for (size_t n = 0; n < clusters.size(); ++n) {
        for (size_t m = 0; m < clusters[n].size(); ++m) {
            ++done[clusters[n][m]];
        }
    }
    for (size_t m = 0; m < numElements; ++m) {
        if (!done[m]) {
            clusters.emplace_back();
            clusters.back().push_back(m);
        }
    }
}

/*
struct TTestClustering
{
    TTestClustering() {
        TDupes src;
        src.push_back(std::make_pair(0, 1));
        src.push_back(std::make_pair(0, 2));
        src.push_back(std::make_pair(3, 4));
        src.push_back(std::make_pair(5, 6));
        src.push_back(std::make_pair(7, 8));
        src.push_back(std::make_pair(9, 8));
        src.push_back(std::make_pair(2, 4));
        TVector<TVector<int> > clusters;
        PairsToClusters(src, &clusters);
        Y_ASSERT(clusters.size() == 3);
    }
} testClustering;
*/

////////////////////////////////////////////////////////////////////////////////////////////////////
static const char* TheVolapukVariants[] = {
    "q|й",
    "w|ц",
    "e|у",
    "r|к",
    "t|е",
    "y|н",
    "u|г",
    "i|ш",
    "o|щ",
    "p|з",
    " |х",
    " |ъ",

    "a|ф",
    "s|ы",
    "d|в",
    "f|а",
    "g|п",
    "h|р",
    "j|о",
    "k|л",
    "l|д",
    " |ж",
    " |'|э",

    "z|я",
    "x|ч",
    "c|с",
    "v|м",
    "b|и",
    "n|т",
    "m|ь",
    " |б",
    " |ю",

    " |ё|`",
    nullptr
};
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool HasEn(const TUtf16String &w) {
    for (size_t n = 0; n < w.size(); ++n)
        if (w[n] >= 'a' && w[n] <= 'z')
            return true;
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void SelectRuEnKeybDuplicates(const TVector<TUtf16String> &src, TDupes *res)
{
    TDupes dupes;
    SelectDuplicates(src, &dupes, TheVolapukVariants);
    for (size_t n = 0; n < dupes.size(); ++n) {
        TUtf16String str1 = src[dupes[n].first];
        TUtf16String str2 = src[dupes[n].first];
        if (!HasEn(str1) && !HasEn(str2))
            continue;
        res->push_back(dupes[n]);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsReorderDuplicates(const TUtf16String &w1, const TUtf16String &w2)
{
    TUtf16String space; space += ' ';
    size_t spacePos = w1.find(' ');
    while (spacePos != TUtf16String::npos) {
        if (w1.substr(spacePos + 1) + space + w1.substr(0, spacePos) == w2)
            return true;
        spacePos = w1.find(' ', spacePos + 1);
    }
    return false;
}

class TReorderShingles: public TShingles
{
public:
    TReorderShingles(const TVector<TUtf16String> &src, TDupes *res)
        : TShingles(src, res)
    {
    }
    bool AreRealDupes(const TUtf16String &w1, const TUtf16String &w2) const override {
        return IsReorderDuplicates(w1, w2);
    }
    void AddAllEvidences(TUtf16String phrase, int id) {
        if (phrase.find(' ') == TUtf16String::npos)
            return;
        TVector<TUtf16String> words;
        Wsplit(phrase.begin(), ' ', &words);
        std::sort(words.begin(), words.end());
        TUtf16String reorder;
        for (size_t n = 0; n < words.size(); ++n) {
            reorder += words[n];
            reorder += ' ';
        }
        AddStrongEvidence(reorder, id);
    }
};

void SelectReorderDupes(const TVector<TUtf16String> &src, TDupes *res)
{
    TReorderShingles shingles(src, res);
    for (size_t n = 0; n < src.size(); ++n, shingles.Next()) {
        if (n % 10000 == 0)
            Cout << n << Endl;
        shingles.AddAllEvidences(src[n], n);
    }
}

