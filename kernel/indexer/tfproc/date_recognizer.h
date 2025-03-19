#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

struct TRecognizedDate {
    int Day;
    int Month;
    int Year;
    int Priority;

    TRecognizedDate()
        : Day(0)
        , Month(0)
        , Year(0)
        , Priority(0)
    {
    }

    TRecognizedDate(int day, int month, int year)
        : Day(day)
        , Month(month)
        , Year(year)
        , Priority(0)
    {
    }

    static void FromString(const TString& s, TRecognizedDate* result);

    TString ToString() const;

    ui64 ToInt64() const {
        return Day + Month*31 + Year*372;
    }

    static void FromInt64(ui64 dt, TRecognizedDate* result) {
        result->Year = static_cast<int>(dt / 372);
        dt %= 372;
        result->Month = static_cast<int>(dt / 31);
        dt %= 31;
        result->Day = static_cast<int>(dt);
    }

    bool operator<(const TRecognizedDate& dt) const;
};

typedef TVector<TRecognizedDate> TRecognizedDates;

class TStreamDateRecognizer {
private:
    class TImpl;
    TImpl* Impl;

public:
    TStreamDateRecognizer(bool useTurkish = false);
    ~TStreamDateRecognizer();
    void Clear();
    void Push(const TString& tokens);
    void Push(const char* token, size_t length);
    bool GetDate(TRecognizedDate* result, bool disallowYearOnly = false);
};

void RecognizeDates(const TString& s, TRecognizedDates* dates);
