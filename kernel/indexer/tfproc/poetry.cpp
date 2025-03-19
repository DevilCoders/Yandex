#include "poetry.h"

#include <library/cpp/charset/codepage.h>
#include <library/cpp/pop_count/popcount.h>
#include <library/cpp/token/token_structure.h>

#include <util/string/split.h>
#include <util/string/cast.h>
#include <util/charset/wide.h>
#include <util/generic/ymath.h>

static bool IsVowel(char ch) {
    return strchr("\xE0\xEE\xF3\xFB\xFD\xFF\xB8\xFE\xE8\xE5" /* "аоуыэяёюие" */, ch) != nullptr;
}

static ui32 JSHash(const char *word) {
    ui32 hash = 1315423911;
    for (; *word; ++word)
        hash ^= (((hash << 5) ^ *word ^ (hash >> 2)));
    return hash;
}

static bool ConvertWord(const wchar16 *tok, char *res, size_t len, size_t &vow_cnt, ui32 &hash) {
    hash = 1315423911;
    vow_cnt = 0;
    for (; len; ++tok, ++res, --len) {
        *res = csYandex.ToLower(WideCharToYandex.Tr(*tok));
        if (IsVowel(*res))
            ++vow_cnt;
        else if (('0' <= *res && *res <= '9') || ('a' <= *res && *res <= 'z'))
            return false;
        hash ^= (((hash << 5) ^ *res ^ (hash >> 2)));
    }
    *res = 0;
    return true;
}


// TWordForces
void TWordForces::Load(const char *file_name) {
    TFileInput fin(file_name);
    TString t = fin.ReadLine();
    size_t cnt = FromString<size_t>(t);
    for (size_t i = 0; i<cnt; ++i) {
        t = fin.ReadLine();
        // Line format is: <word>\t<vowel_count>\t*<prob_i>
        TVector<TString> arr;
        Split(t.c_str(), "\t ", arr);
        // Read the word
        TString word = arr[0];
        // Read the count
        size_t v_cnt = FromString<size_t>(arr[1]);
        // Read probabilities
        TVector<float> forces;
        for (size_t j = 0; j<v_cnt; ++j)
            forces.push_back(FromString<float>(arr[j+2]));
        // Select max probabilities
        float prob = -1.0f;
        size_t m_cnt = 0;
        for (TVector<float>::const_iterator it = forces.begin(), end = forces.end(); it!=end; ++it)
            if (prob==*it)
                ++m_cnt;
            else if (prob<*it) {
                prob = *it;
                m_cnt = 1;
            }
        ui32 val = 0, mask = 1;
        for (TVector<float>::const_iterator it = forces.begin(), end = forces.end(); it!=end; ++it, mask <<= 1)
            if (prob==*it)
                val |= mask;
        if (Map[JSHash(word.c_str())]!=0)
            Cerr << "Warning: hash collision on word \'" << word << '\'' << Endl;
        Map[JSHash(word.c_str())] = val;
    }
}

void TWordForces::GetForces(const TWideToken &tok, TVector<float>* res) {
    static const size_t MAX_WORD_LEN = 20;
    res->clear();
    res->reserve(MAX_WORD_LEN);
    if (tok.Leng>MAX_WORD_LEN) {
        res->resize(MAX_WORD_LEN);
        return;
    }
    char Buf[MAX_WORD_LEN+1];
    size_t cnt;
    ui32 hash;
    if (!ConvertWord(tok.Token, Buf, tok.Leng, cnt, hash)) {
        res->resize(MAX_WORD_LEN);
        return;
    }
    res->resize(cnt);
    std::fill(res->begin(), res->end(), 0.0f);
    TMapType::const_iterator it = Map.find(hash);
    size_t val = it!=Map.end() ? it->second : 0;
    size_t prob_forces = PopCount(val);
    float prob = prob_forces ? 1.0f / prob_forces : 0.0f;
    for (size_t i = 0; val && i<cnt; ++i, val >>= 1)
        if (val&1)
            (*res)[i] = prob;
}


// Rithm constants
static const size_t RithmSize[RT_COUNT] = { 2, 8, 2, 8, 3, 3, 3 };
static const float RithmMask[RT_COUNT][8] = {
    {1.0f, -0.25f},
    {1.0f, -0.25f, 1.0f, -0.25f, -0.25f, -0.25f, 1.0f, -0.25f},
    {-0.25f, 1.0f},
    {-0.25f, 1.0f, -0.25f, 1.0f, -0.25f, -0.25f, -0.25f, 1.0f},
    {1.0f, -0.25f, -0.25f},
    {-0.25f, 1.0f, -0.25f},
    {-0.25f, -0.25f, 1.0f}
};


// Check if two rithms are replacable
static bool IsEqualRithm(TRithmType r1, TRithmType r2) {
    if (r1==r2)
        return true;
    if ((r1==RT_HOREY && r2==RT_HOREY_M) || (r1==RT_HOREY_M && r2==RT_HOREY))
        return true;
    if ((r1==RT_YAMB && r2==RT_YAMB_M) || (r1==RT_YAMB_M && r2==RT_YAMB))
        return true;
    return false;
}


// TStrophePoetryMatcher
TRithmType TStrophePoetryMatcher::Calculate() {
    if (Forces.empty() )
        return RT_EMPTY;    // Empty line
    if (WordCount < 2 || Forces.size() >= 20)
        return RT_ABSENT;   // Too short or too long line
    size_t size = Forces.size(), neg_cnt = 0;
    for (size_t i = 0; i < size; ++i) {
        ProcessVowel(i, Forces[i]);     // Process each vowel
        if (Forces[i] < 0)
            ++neg_cnt;
    }
    if (neg_cnt >= size / 2) {          // If we know only few words, then return
        std::fill(Rithms, Rithms+RT_COUNT, 0.0f);
        return RT_FEW_INFO;
    }
    else {
        // Clean rithms, that contains less than 2 parts
        for (TRithmType j = RT_FIRST; j<RT_COUNT; j = static_cast<TRithmType>(j+1))
            Rithms[j] = std::max(Counts[j]>1 ? Rithms[j]/Counts[j] : 0.0f, 0.0f);
        static const float RITHM_BORDER = 0.3f;
        static const float MIN_RITHM_DIFF = 0.1f;
        TRithmType res = RT_UNKNOWN;
        // Select the most match rithm
        for (TRithmType i = RT_FIRST; i<RT_COUNT; i = static_cast<TRithmType>(i+1))
            if ((Rithms[i] >= RITHM_BORDER) && (res == RT_UNKNOWN || Rithms[i] > Rithms[res]))
                res = i;
        // Flush if two non-similar rithms took high marks
        for (TRithmType i = RT_FIRST; res<RT_COUNT && i<RT_COUNT; i = static_cast<TRithmType>(i+1))
            if (!IsEqualRithm(i, res) && fabs(Rithms[res] - Rithms[i])<MIN_RITHM_DIFF)
                res = RT_UNKNOWN;
        return res;
    }
}

void TStrophePoetryMatcher::ProcessVowel(size_t v_num, float prob) {
    if (prob<0.0f)
        prob = 0.0f;
    for (TRithmType k = RT_FIRST; k<RT_COUNT; k = static_cast<TRithmType>(k+1)) {
        size_t t = v_num%RithmSize[k];
        if (RithmMask[k][t]==1.0f)
            ++Counts[k];
        Rithms[k] += (prob*RithmMask[k][t]);
    }
}


// TPoetryMatcher
void TPoetryMatcher::FlushLine() {
    TRithmType type = Strophe.Calculate();
    if (type!=RT_EMPTY) {
        if (type<RT_COUNT)      // If rithm determined for sure, then store it
            Values.push_back(std::make_pair(type, Strophe.GetRithms()[type]));
        else {
                                // If the rithm is absent, then push some zeros
            size_t cnt = type==RT_ABSENT ? std::max(Strophe.GetWordCount() / 4, static_cast<size_t>(1)) : 1;
            for (size_t i = 0; i<cnt; ++i)
                Values.push_back(std::make_pair(type, 0.0f));
        }
    }
    Strophe.Clear();            // Prepare for the next line
}

void TPoetryMatcher::OnZoneBreak() {
    FlushLine();
    Values.push_back(std::make_pair(RT_ZONE_BREAK, 0.0f));
}

float TPoetryMatcher::PoetryValue() const {
    if (Values.empty())
        return 0.0f;
    float val = 0.0f;
    size_t line_cnt = 0;
    for (size_t i = 0, cnt = Values.size(); i<cnt; ) {
        // Count all real lines
        if (Values[i].first != RT_ZONE_BREAK)
            ++line_cnt;
        size_t j;
                                // Scan for equal rithms
        for (j = i+1; j<cnt && IsEqualRithm(Values[j].first, Values[i].first); ++j) {
            if (Values[j].first != RT_ZONE_BREAK)
                ++line_cnt;
        }
        if (j-i>=4)             // We count only "long" continuous strophes with equal rithms
            for (size_t k = i; k<j; ++k)
                val += Values[k].second;
        i = j;
    }
    return line_cnt ? val/line_cnt : 0.0f;      // Return average probability
}

float TPoetryMatcher::PoetryValue2() const {
    if (Values.empty())
        return 0.0f;
    float res = 0.0f;
    for (size_t i = 0, cnt = Values.size(); i<cnt; ) {
        size_t j;
                                // Scan for equal rithms
        for (j = i+1; j<cnt && IsEqualRithm(Values[j].first, Values[i].first); ++j) {
        }
        if (j-i>=4) {           // If ther is at least 4 strophes, then select max 4-strophes
            float sum = 0.0f;
            for (size_t k = i; k<i+4; ++k)
                sum += Values[k].second;
            res = std::max(res, sum);
            for (size_t k = i+4; k<j; ++k) {
                sum += Values[k].second;
                sum -= Values[k-4].second;
                res = std::max(res, sum);
            }
        }
        i = j;
    }
    return res/4;               // Return average probability
}
