#include <cstring>
#include <cctype>

#include <algorithm>

#include <library/cpp/charset/ci_string.h>
#include <library/cpp/charset/recyr.hh>
#include <util/draft/datetime.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/yexception.h>
#include <util/generic/singleton.h>
#include <util/generic/ymath.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/stream/output.h>
#include <library/cpp/deprecated/split/split_iterator.h>

#include "date_recognizer.h"

static const TString TURKISH_LETTER_STUB("$");

struct TDateToken {
    enum TType {tNumber, tEnglishMonth, tRussianMonth, tTurkishMonth, tDash, tPoint, tUnknown, tEof, tSlash, tComma, tPipe, tColon};

    TType Type;
    int Number;

    void Swap(TDateToken& token) {
        std::swap(Type, token.Type);
        std::swap(Number, token.Number);
    }

    static TString TypeToString(TType type) {
        switch (type) {
            case tNumber:
                return "tNumber";
            case tEnglishMonth:
                return "tEnglishMonth";
            case tRussianMonth:
                return "tRussianMonth";
            case tTurkishMonth:
                return "tTurkishMonth";
            case tDash:
                return "tDash";
            case tSlash:
                return "tSlash";
            case tPoint:
                return "tPoint";
            case tComma:
                return "tComma";
            case tUnknown:
                return "tUnknown";
            case tColon:
                return "tColon";
            case tPipe:
                return "tPipe";
            case tEof:
                return "tEof";
            default:
                ythrow yexception() << "unknown token type";
        }
    }

    TString ToString() const {
        return TString("(") + TypeToString(Type) + ", " + ::ToString<int>(Number) + ")";
    }

    bool operator==(const TDateToken& token) const {
        return token.Type == Type;
    }

    bool operator!=(const TDateToken& token) const {
        return token.Type != Type;
    }
};

class TDateTokenizer {
private:
    const char* Now;
    const char* End;
    const bool UseTurkish;

    TDateToken NextToken;
    TString Buffer;

    static bool IsRussianLetter(char ch) {
        if ( (ch >= '\xE0' /* 'а' */) && (ch <= '\xFF' /* 'я' */) )
            return true;
        if ( (ch >= '\xC0' /* 'А' */) && (ch <= '\xDF' /* 'Я' */) )
            return true;
        return false;
    }

    static bool IsEnglishLetter(char ch) {
        if ( (ch >= 'a') && (ch <= 'z') )
            return true;
        if ( (ch >= 'A') && (ch <= 'Z') )
            return true;
        return false;
    }

    static bool IsDigit(char ch) {
        return (ch >= '0') && (ch <= '9');
    }

    static bool IsSignificant(char ch) {
        if (IsDigit(ch))
            return true;
        if (IsRussianLetter(ch))
            return true;
        if (IsEnglishLetter(ch))
            return true;
        if ('.' == ch)
            return true;
        if ('-' == ch)
            return true;
        if ('/' == ch)
            return true;
        if (',' == ch)
            return true;
        if (':' == ch)
            return true;
        if ('|' == ch)
            return true;
        return false;
    }

    typedef THashMap<TCiString, int> TStrokaCIHash;

    static void InsertUtf8(TStrokaCIHash& hash, TStringBuf key, int value) {
        TCiString ykey;
        Recode(CODES_UTF8, CODES_YANDEX, key, ykey);
        hash.emplace(std::move(ykey), value);
    }

    class TEnglishMonths {
    public:
        TStrokaCIHash Hash;
        TEnglishMonths() {
            Hash.insert( TStrokaCIHash::value_type("jan", 1) );
            Hash.insert( TStrokaCIHash::value_type("feb", 2) );
            Hash.insert( TStrokaCIHash::value_type("mar", 3) );
            Hash.insert( TStrokaCIHash::value_type("apr", 4) );
            Hash.insert( TStrokaCIHash::value_type("may", 5) );
            Hash.insert( TStrokaCIHash::value_type("jun", 6) );
            Hash.insert( TStrokaCIHash::value_type("jul", 7) );
            Hash.insert( TStrokaCIHash::value_type("jly", 7) );
            Hash.insert( TStrokaCIHash::value_type("aug", 8) );
            Hash.insert( TStrokaCIHash::value_type("sep", 9) );
            Hash.insert( TStrokaCIHash::value_type("sept", 9) );
            Hash.insert( TStrokaCIHash::value_type("oct", 10) );
            Hash.insert( TStrokaCIHash::value_type("nov", 11) );
            Hash.insert( TStrokaCIHash::value_type("dec", 12) );
            Hash.insert( TStrokaCIHash::value_type("january", 1) );
            Hash.insert( TStrokaCIHash::value_type("february", 2) );
            Hash.insert( TStrokaCIHash::value_type("march", 3) );
            Hash.insert( TStrokaCIHash::value_type("april", 4) );
            Hash.insert( TStrokaCIHash::value_type("may", 5) );
            Hash.insert( TStrokaCIHash::value_type("june", 6) );
            Hash.insert( TStrokaCIHash::value_type("july", 7) );
            Hash.insert( TStrokaCIHash::value_type("august", 8) );
            Hash.insert( TStrokaCIHash::value_type("september", 9) );
            Hash.insert( TStrokaCIHash::value_type("october", 10) );
            Hash.insert( TStrokaCIHash::value_type("november", 11) );
            Hash.insert( TStrokaCIHash::value_type("december", 12) );
        }
    };

    static bool LookupEnglishMonth(const TString& s, int* number) {
        const TEnglishMonths* month = Singleton<TEnglishMonths>();
        TStrokaCIHash::const_iterator toMonth = month->Hash.find(s);
        if (toMonth != month->Hash.end()) {
            *number = toMonth->second;
            return true;
        } else {
            return false;
        }
    }

    class TRussianMonths {
    public:
        TStrokaCIHash Hash;
        TRussianMonths() {
            InsertUtf8(Hash, "янв", 1);
            InsertUtf8(Hash, "фев", 2);
            InsertUtf8(Hash, "февр", 2);
            InsertUtf8(Hash, "мар", 3);
            InsertUtf8(Hash, "апр", 4);
            InsertUtf8(Hash, "май", 5);
            InsertUtf8(Hash, "июн", 6);
            InsertUtf8(Hash, "июл", 7);
            InsertUtf8(Hash, "авг", 8);
            InsertUtf8(Hash, "сен", 9);
            InsertUtf8(Hash, "сент", 9);
            InsertUtf8(Hash, "окт", 10);
            InsertUtf8(Hash, "нбр", 11);
            InsertUtf8(Hash, "ноя", 11);
            InsertUtf8(Hash, "нояб", 11);
            InsertUtf8(Hash, "дек", 12);
            InsertUtf8(Hash, "январь", 1);
            InsertUtf8(Hash, "января", 1);
            InsertUtf8(Hash, "февраль", 2);
            InsertUtf8(Hash, "февраля", 2);
            InsertUtf8(Hash, "март", 3);
            InsertUtf8(Hash, "марта", 3);
            InsertUtf8(Hash, "апрель", 4);
            InsertUtf8(Hash, "апреля", 4);
            InsertUtf8(Hash, "май", 5);
            InsertUtf8(Hash, "мая", 5);
            InsertUtf8(Hash, "июнь", 6);
            InsertUtf8(Hash, "июня", 6);
            InsertUtf8(Hash, "июль", 7);
            InsertUtf8(Hash, "июля", 7);
            InsertUtf8(Hash, "август", 8);
            InsertUtf8(Hash, "августа", 8);
            InsertUtf8(Hash, "сентябрь", 9);
            InsertUtf8(Hash, "сентября", 9);
            InsertUtf8(Hash, "октябрь", 10);
            InsertUtf8(Hash, "октября", 10);
            InsertUtf8(Hash, "ноябрь", 11);
            InsertUtf8(Hash, "ноября", 11);
            InsertUtf8(Hash, "декабрь", 12);
            InsertUtf8(Hash, "декабря", 12);
        }
    };

    static bool LookupRussianMonth(const TString& s, int* number) {
        const TRussianMonths* month = Singleton<TRussianMonths>();
        TStrokaCIHash::const_iterator toMonth = month->Hash.find(s);
        if (toMonth != month->Hash.end()) {
            *number = toMonth->second;
            return true;
        } else {
            return false;
        }
    }

    class TTurkishMonths {
    public:
        TStrokaCIHash Hash;
        TTurkishMonths() {
            Hash.insert( TStrokaCIHash::value_type("ocak", 1) );
            // CalcNext checks for first eng letter
            Hash.insert( TStrokaCIHash::value_type(/*TURKISH_LETTER_STUB + */"ubat", 2) );
            Hash.insert( TStrokaCIHash::value_type("subat", 2) );
            Hash.insert( TStrokaCIHash::value_type("mart", 3) );
            Hash.insert( TStrokaCIHash::value_type("nisan", 4) );
            Hash.insert( TStrokaCIHash::value_type("may" + TURKISH_LETTER_STUB + "s", 5) );
            Hash.insert( TStrokaCIHash::value_type("haziran", 6) );
            Hash.insert( TStrokaCIHash::value_type("temmuz", 7) );
            Hash.insert( TStrokaCIHash::value_type("a" + TURKISH_LETTER_STUB + "ustos", 8) );
            Hash.insert( TStrokaCIHash::value_type("agustos", 8) );
            Hash.insert( TStrokaCIHash::value_type("eyl" + TURKISH_LETTER_STUB + "l", 9) );
            Hash.insert( TStrokaCIHash::value_type("ekim", 10) );
            Hash.insert( TStrokaCIHash::value_type("kas" + TURKISH_LETTER_STUB + "m", 11) );
            Hash.insert( TStrokaCIHash::value_type("aral" + TURKISH_LETTER_STUB + "k", 12) );
        }
    };

    static bool LookupTurkishMonth(const TString& s, int* number) {
        const TTurkishMonths* month = Singleton<TTurkishMonths>();
        TStrokaCIHash::const_iterator toMonth = month->Hash.find(s);
        if (toMonth != month->Hash.end()) {
            *number = toMonth->second;
            return true;
        } else {
            return false;
        }
    }

    void CalcNext() {
        while ((Now < End) && !IsSignificant(*Now))
            ++Now;
        if (Now != End) {
            if (IsDigit(*Now)) {
                NextToken.Number = 0;
                while ((Now < End) && IsDigit(*Now)) {
                    NextToken.Number = NextToken.Number*10 + (*Now - '0');
                    ++Now;
                }
                NextToken.Type = TDateToken::tNumber;
            } else if (IsRussianLetter(*Now)) {
                Buffer.clear();
                while ((Now < End) && IsRussianLetter(*Now)) {
                    Buffer += *Now;
                    ++Now;
                }
                if (LookupRussianMonth(Buffer, &NextToken.Number))
                    NextToken.Type = TDateToken::tRussianMonth;
                else
                    NextToken.Type = TDateToken::tUnknown;
            } else if (IsEnglishLetter(*Now)) {
                NextToken.Type = TDateToken::tUnknown;
                Buffer.clear();
                while ((Now < End) && IsEnglishLetter(*Now)) {
                    Buffer += *Now;
                    ++Now;
                }
                if (!UseTurkish) {
                    if (LookupEnglishMonth(Buffer, &NextToken.Number))
                        NextToken.Type = TDateToken::tEnglishMonth;
                } else if (LookupTurkishMonth(Buffer, &NextToken.Number))
                    // check turkish before english ("may" in intersection)
                    NextToken.Type = TDateToken::tTurkishMonth;
                else {
                    if (Now < End) {
                        // TODO: rewrite to wchar16
                        // hack for turkish letter with cp-1251
                        TString turkBuffer = Buffer + TURKISH_LETTER_STUB;
                        const char* nowForTurkWord = Now + 1;
                        while ((nowForTurkWord < End) && IsEnglishLetter(*nowForTurkWord)) {
                            turkBuffer += *nowForTurkWord;
                            ++nowForTurkWord;
                        }
                        if (LookupTurkishMonth(turkBuffer, &NextToken.Number)) {
                            NextToken.Type = TDateToken::tTurkishMonth;
                            Now = nowForTurkWord;
                        }
                    }
                    if (NextToken.Type == TDateToken::tUnknown &&
                        LookupEnglishMonth(Buffer, &NextToken.Number))
                    {
                        NextToken.Type = TDateToken::tEnglishMonth;
                    }
                }
            } else if ('-' == *Now) {
                NextToken.Type = TDateToken::tDash;
                ++Now;
            } else if ('.' == *Now) {
                NextToken.Type = TDateToken::tPoint;
                ++Now;
            } else if (',' == *Now) {
                NextToken.Type = TDateToken::tComma;
                ++Now;
            } else if (':' == *Now) {
                NextToken.Type = TDateToken::tColon;
                ++Now;
            } else if ('/' == *Now) {
                NextToken.Type = TDateToken::tSlash;
                ++Now;
            } else if ('|' == *Now) {
                NextToken.Type = TDateToken::tPipe;
                ++Now;
            } else {
                NextToken.Type = TDateToken::tUnknown;
                ++Now;
            }
        } else {
            NextToken.Type = TDateToken::tEof;
        }
    }

public:
    TDateTokenizer(const char* begin, const char* end, bool useTurkish = false)
        : Now(begin)
        , End(end)
        , UseTurkish(useTurkish)
    {

    }

    TDateTokenizer(const char* begin, bool useTurkish = false)
        : Now(begin)
        , End(begin + strlen(begin))
        , UseTurkish(useTurkish)
    {
    }

    TDateTokenizer(bool useTurkish = false)
        : Now(nullptr)
        , End(nullptr)
        , UseTurkish(useTurkish)
    {
    }

    void Reset(const char* begin, const char* end) {
        Now = begin;
        End = end;
    }

    const TDateToken* Get() {
        CalcNext();
        return &NextToken;
    }
};

class IDatePattern {
public:
    typedef TVector<TDateToken::TType> TDateTokenTypes;
    virtual void GetSubstring(TDateTokenTypes* result) const = 0;
    typedef TVector<TDateToken> TDateTokens;
    virtual bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const = 0;

    static bool CheckDay(int* day) {
        if (*day <= 0)
            return false;
        if (*day > 31)
            return false;
        return true;
    }

    static bool CheckMonth(int* month) {
        if (*month <= 0)
            return false;
        if (*month > 12)
            return false;
        return true;
    }

    const int MIN_FULL_YEAR = 1994;
    // year can't be more than current year
    const int MAX_FULL_YEAR = static_cast<int>(NDatetime::TSimpleTM::CurrentUTC().RealYear()) + 1;
    const int MAX_FULL_YEAR_SHORT = MAX_FULL_YEAR - 2000;

    bool CheckYear(int* year) const {
        if (*year < MAX_FULL_YEAR_SHORT)
            *year += 2000;
        if (*year < 100)
            *year += 1900;
        if (*year <= MIN_FULL_YEAR)
            return false;
        if (*year >= MAX_FULL_YEAR)
            return false;
        return true;
    }

    static bool CheckDate(TRecognizedDate* /*result*/) {
        return true;
    }

    bool GetDate(const TDateToken* tokens, TRecognizedDate* result) const {
        try {
            if (GetDateInt(tokens, result)) {
                result->Priority = GetPriority();
                return CheckDay(&result->Day) && CheckMonth(&result->Month) && CheckYear(&result->Year) && CheckDate(result);
            } else {
                return false;
            }
        } catch (...) {
            Cerr << CurrentExceptionMessage() << Endl;
            return false;
        }
    }

    virtual int GetPriority() const {
        return 2;
    }

    virtual ~IDatePattern() {
    }
};

class TYearPattern : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        if (tokens[0].Number >= MIN_FULL_YEAR && tokens[0].Number <= MAX_FULL_YEAR) {
            result->Day = 1;
            result->Month = 1;
            result->Year = tokens[0].Number;
            return true;
        } else {
            return false;
        }
    }

    int GetPriority() const override {
        return 1;
    }
};

class TSimplePattern1 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tNumber, TDateToken::tPoint, TDateToken::tNumber, TDateToken::tPoint, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[0].Number;
        result->Month = tokens[2].Number;
        result->Year = tokens[4].Number;
        return true;
    }

    int GetPriority() const override {
        return 7;
    }
};

class TSimplePattern2 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tNumber, TDateToken::tSlash, TDateToken::tNumber, TDateToken::tSlash, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[0].Number;
        result->Month = tokens[2].Number;
        result->Year = tokens[4].Number;
        return true;
    }
};

class TSimplePattern3 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tNumber, TDateToken::tDash, TDateToken::tNumber, TDateToken::tDash, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[0].Number;
        result->Month = tokens[2].Number;
        result->Year = tokens[4].Number;
        return true;
    }
};

class TSimplePattern4 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tNumber, TDateToken::tDash, TDateToken::tNumber, TDateToken::tDash, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Year = tokens[0].Number;
        result->Month = tokens[2].Number;
        result->Day = tokens[4].Number;
        return true;
    }
};

class TRusPattern1 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tNumber, TDateToken::tRussianMonth, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[0].Number;
        result->Month = tokens[1].Number;
        result->Year = tokens[2].Number;
        return true;
    }

    int GetPriority() const override {
        return 10;
    }
};

class TRusPattern2 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tRussianMonth, TDateToken::tNumber, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[1].Number;
        result->Month = tokens[0].Number;
        result->Year = tokens[2].Number;
        return true;
    }

    int GetPriority() const override {
        return 10;
    }
};

class TRusPattern3 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tNumber, TDateToken::tRussianMonth, TDateToken::tComma, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[0].Number;
        result->Month = tokens[1].Number;
        result->Year = tokens[3].Number;
        return true;
    }
};

class TRusPattern4 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tRussianMonth, TDateToken::tNumber, TDateToken::tComma, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[1].Number;
        result->Month = tokens[0].Number;
        result->Year = tokens[3].Number;
        return true;
    }
};

class TEngPattern1 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tNumber, TDateToken::tEnglishMonth, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[0].Number;
        result->Month = tokens[1].Number;
        result->Year = tokens[2].Number;
        return true;
    }
};

class TEngPattern2 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tEnglishMonth, TDateToken::tNumber, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[1].Number;
        result->Month = tokens[0].Number;
        result->Year = tokens[2].Number;
        return true;
    }
};

class TEngPattern3 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tEnglishMonth, TDateToken::tNumber, TDateToken::tComma, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[1].Number;
        result->Month = tokens[0].Number;
        result->Year = tokens[3].Number;
        return true;
    }
};

class TTurkPattern1 : public IDatePattern {
public:
    void GetSubstring(TDateTokenTypes* result) const override {
        TDateTokenTypes({TDateToken::tNumber, TDateToken::tTurkishMonth, TDateToken::tNumber}).swap(*result);
    }

    bool GetDateInt(const TDateToken* tokens, TRecognizedDate* result) const override {
        result->Day = tokens[0].Number;
        result->Month = tokens[1].Number;
        result->Year = tokens[2].Number;
        return true;
    }
};

class TKMPRecognizer {
private:
    class TKMPStreamMatcherCallback : public TKMPStreamMatcher<TDateToken>::ICallback {
    private:
        const IDatePattern* Pattern;
        TRecognizedDates* Result;

    public:
        TKMPStreamMatcherCallback(const IDatePattern* pattern, TRecognizedDates* result)
            : Pattern(pattern)
            , Result(result)
        {
        }

        void OnMatch(const TDateToken* begin, const TDateToken* /*end*/) override {
            TRecognizedDate date;
            /*
            Cerr << "[";
            for (const TDateToken* t = begin; t < end; ++t)
                Cerr << t->ToString() << " ";
            Cerr << "]" << Endl;
            */
            if (Pattern->GetDate(begin, &date)) {
                // Cerr << "Reco: " << date.ToString() << Endl;
                Result->push_back(date);
            }
        }
    };

    TKMPStreamMatcherCallback Callback;
    TAutoPtr< TKMPStreamMatcher<TDateToken> > Matcher;

public:
    TKMPRecognizer(const IDatePattern* pattern, TRecognizedDates* result)
        : Callback(pattern, result)
    {
        IDatePattern::TDateTokenTypes types;
        pattern->GetSubstring(&types);
        IDatePattern::TDateTokens tokens(types.size());
        for (size_t i = 0; i < types.size(); ++i)
            tokens[i].Type = types[i];
        Matcher.Reset(new TKMPStreamMatcher<TDateToken>(tokens.begin(), tokens.end(), &Callback));
    }

    void Push(const TDateToken* token) {
        Matcher->Push(*token);
    }

    void Clear() {
        Matcher->Clear();
    }
};

class TDateRecognizer {
private:
    typedef TVector< TAutoPtr<IDatePattern> > IDatePatterns;
    IDatePatterns Patterns;

    typedef TVector< TAutoPtr<TKMPRecognizer> > TKMPRecognizers;
    TKMPRecognizers Recognizers;

    TRecognizedDates Dates;

public:
    TDateRecognizer(bool useTurkish = false) {
        Patterns.push_back(new TSimplePattern1());
        Patterns.push_back(new TSimplePattern2());
        Patterns.push_back(new TSimplePattern3());
        Patterns.push_back(new TSimplePattern4());
        Patterns.push_back(new TYearPattern());
        Patterns.push_back(new TRusPattern1());
        Patterns.push_back(new TRusPattern2());
        Patterns.push_back(new TRusPattern3());
        Patterns.push_back(new TRusPattern4());
        Patterns.push_back(new TEngPattern1());
        Patterns.push_back(new TEngPattern2());
        Patterns.push_back(new TEngPattern3());
        if (useTurkish)
            Patterns.push_back(new TTurkPattern1());

        for (IDatePatterns::const_iterator toPattern = Patterns.begin(); toPattern != Patterns.end(); ++toPattern)
            Recognizers.push_back( new TKMPRecognizer(toPattern->Get(), &Dates) );
    }

    void Clear() {
        for (TKMPRecognizers::const_iterator toR = Recognizers.begin(); toR != Recognizers.end(); ++toR)
            (*toR)->Clear();
        Dates.clear();
    }

    void Push(const TDateToken* token) {
        // Cerr << token->ToString() << Endl;
        for (TKMPRecognizers::const_iterator toRecognizer = Recognizers.begin(); toRecognizer != Recognizers.end(); ++toRecognizer)
            (*toRecognizer)->Push(token);
    }

    TRecognizedDates& GetDates() {
        return Dates;
    }

    bool SelectDate(TRecognizedDate* result, bool disallowYearOnly = false) {
        if (!Dates.empty()) {
            double sum = 0;
            double weight = 0;
            bool foundValid = false;
            for (size_t iDate = 0; iDate < Dates.size(); ++iDate) {
                if (disallowYearOnly && Dates[iDate].Priority <= 1)
                    continue;
                sum += ((double)Dates[iDate].ToInt64())*Dates[iDate].Priority;
                weight += Dates[iDate].Priority;
                foundValid = true;
                // Cerr << Dates[iDate].ToString() << " ";
            }
            if (!foundValid)
                return false;
            sum /= weight;
            size_t iBest = (size_t)-1;
            double diffBest = 1E10;
            for (size_t i = 0; i < Dates.size(); ++i) {
                double diff = fabs(Dates[i].ToInt64() - sum);
                if (diff < diffBest) {
                    diffBest = diff;
                    iBest = i;
                }
            }
            *result = Dates[iBest];
            // Cerr << " | " << result->ToString() << Endl;
            /*
            std::sort(Dates.begin(), Dates.end());
            *result = Dates[Dates.size() / 2];
            */
            return true;
        } else {
            return false;
        }
    }
};

TString TRecognizedDate::ToString() const {
    return Sprintf("%02d.%02d.%04d", Day, Month, Year);
}

void TRecognizedDate::FromString(const TString& s, TRecognizedDate* result) {
    const static TSplitDelimiters DELIMS(".");
    const TDelimitersSplit split(s, DELIMS);
    TDelimitersSplit::TIterator it = split.Iterator();
    result->Day = ::FromString<int>(it.NextString());
    result->Month = ::FromString<int>(it.NextString());
    result->Year = ::FromString<int>(it.NextString());
    if (!it.Eof())
        ythrow yexception() << "bad date string";
}

bool TRecognizedDate::operator<(const TRecognizedDate& dt) const {
    if (Priority != dt.Priority)
        return Priority < dt.Priority;
    if (Year != dt.Year)
        return Year < dt.Year;
    if (Month != dt.Month)
        return Month < dt.Month;
    if (Day != dt.Day)
        return Day < dt.Day;
    return false;
}

class TStreamDateRecognizer::TImpl {
private:
    TDateRecognizer Recognizer;
    TDateTokenizer Tokenizer;

    void PushTokens() {
        const TDateToken* token = Tokenizer.Get();
        while (TDateToken::tEof != token->Type) {
            Recognizer.Push(token);
            token = Tokenizer.Get();
        }
    }

public:
    TImpl(bool useTurkish = false)
        : Recognizer(useTurkish)
        , Tokenizer(useTurkish)
    {
    }

    void Clear() {
        Recognizer.Clear();
    }
    void Push(const TString& tokens) {
        Tokenizer.Reset(tokens.data(), tokens.data() + tokens.size());
        PushTokens();
    }
    void Push(const char* s, size_t len) {
        // Cerr << TString(s, len) << Endl;
        Tokenizer.Reset(s, s + len);
        PushTokens();
    }
    bool GetDate(TRecognizedDate* result, bool disallowYearOnly = false) {
        return Recognizer.SelectDate(result, disallowYearOnly);
    }
};

TStreamDateRecognizer::TStreamDateRecognizer(bool useTurkish)
    : Impl(new TImpl(useTurkish))
{
}

TStreamDateRecognizer::~TStreamDateRecognizer() {
    delete Impl;
}

void TStreamDateRecognizer::Clear() {
    Impl->Clear();
}

void TStreamDateRecognizer::Push(const TString& tokens) {
    Impl->Push(tokens);
}

void TStreamDateRecognizer::Push(const char* s, size_t len) {
    Impl->Push(s, len);
}

bool TStreamDateRecognizer::GetDate(TRecognizedDate* result, bool disallowYearOnly /*= false*/) {
    return Impl->GetDate(result, disallowYearOnly);
}

void RecognizeDates(const TString& s, TRecognizedDates* dates, bool useTurkish = false) {
    TDateRecognizer recognizer(useTurkish);

    TDateTokenizer tokenizer(s.begin(), s.end(), useTurkish);
    const TDateToken* token = tokenizer.Get();
    while (TDateToken::tEof != token->Type) {
        recognizer.Push(token);
        token = tokenizer.Get();
    }

    dates->swap(recognizer.GetDates());
}
