#include "dates.h"
#include <util/datetime/systime.h>
#include <util/memory/tempbuf.h>
#include <library/cpp/charset/wide.h>

namespace NForumsImpl {

static void FillSpecialDate(time_t date, TRecognizedDate* recognDate)
{
    struct tm tmbuf;
    GmTimeR(&date, &tmbuf);
    recognDate->Day = tmbuf.tm_mday;
    recognDate->Month = 1 + tmbuf.tm_mon;
    recognDate->Year = 1900 + tmbuf.tm_year;
    recognDate->Priority = 100;
}

template<size_t n>
inline bool HasCaseInsensitiveSubstring(const TWtringBuf& large, const wchar16 (&substr)[n])
{
    size_t pos = large.find(TWtringBuf(substr + 1, n - 1));
    if (pos != TWtringBuf::npos && pos && (large[pos - 1] == substr[0] || large[pos - 1] == substr[0] - 0x20))
        return true;
    return false;
}

void TForumDateRecognizer::OnDateText(const wchar16* text, unsigned len)
{
    if (NextDateIsRegistration || SpecialRecognizer.Recognized() || RecognizedDate.ToInt64())
        return;
    while (len && (IsSpace(*text) || *text == ','))
        ++text, --len;
    if (!len)
        return;
    static const wchar16 registeredRus1[] = {0x437, 0x430, 0x440, 0x435, 0x433, 0x438, 0x441, 0x442, 0x440, 0x438, 0x440, 0x43E, 0x432, 0x430, 0x43D};
    static const wchar16 registeredRus2[] = {0x440, 0x435, 0x433, 0x438, 0x441, 0x442, 0x440, 0x430, 0x446, 0x438, 0x44F};
    TWtringBuf input(text, len);
    if (HasCaseInsensitiveSubstring(input, registeredRus1) ||
        HasCaseInsensitiveSubstring(input, registeredRus2))
    {
        NextDateIsRegistration = true;
        return;
    }
    SpecialRecognizer.RunFSM(text, text + len);
    if (SpecialRecognizer.Recognized()) {
        FillSpecialDate(DocDate - SpecialRecognizer.GetDeltaSeconds(), &SpecialDate);
        return;
    }
    TTempArray<char> yandexTempBuf(len);
    WideToChar(text, len, yandexTempBuf.Data(), CODES_YANDEX);
    char* buffer = yandexTempBuf.Data();
    buffer[0] = csYandex.ToLower(buffer[0]);
    unsigned i;
    for (i = 0; i < len; i++)
        if (text[i] == '-')
            break;
    if (i < len) {
        unsigned dashBeforeMonth = i;
        for (++i; i < len && text[i] != '-'; i++)
            if (text[i] != ' ' && !IsAlpha(text[i]))
                break;
        if (i < len && text[i] == '-') {
            buffer[dashBeforeMonth] = ' ';
            buffer[i] = ' ';
        }
    }
    // DateRecognizer assumes that XX/YY/ZZ is DD/MM/YY, not MM/DD/YY
    // here we just collect all such dates as dates with priority 0,
    // DetectDateAmerican() will look at the entire array
    if (len >= 8 && (buffer[2] == '/' || buffer[2] == '-') &&
        (buffer[5] == '/' || buffer[5] == '-') &&
        IsCommonDigit(buffer[0]) && IsCommonDigit(buffer[1]) &&
        IsCommonDigit(buffer[3]) && IsCommonDigit(buffer[4]) &&
        IsCommonDigit(buffer[6]) && IsCommonDigit(buffer[7]))
    {
        SpecialDate.Day = (buffer[0] - '0') * 10 + (buffer[1] - '0');
        SpecialDate.Month = (buffer[3] - '0') * 10 + (buffer[4] - '0');
        int year = (buffer[6] - '0') * 10 + (buffer[7] - '0');
        if (len >= 10 && IsCommonDigit(buffer[8]) && IsCommonDigit(buffer[9]))
            year = year * 100 + (buffer[8] - '0') * 10 + (buffer[9] - '0');
        else
            year += 2000;
        SpecialDate.Year = year;
        SpecialDate.Priority = 0;
    } else {
        DateRecognizer.Push(buffer, len);
        DateRecognizer.GetDate(&RecognizedDate, true);
    }
}

} // namespace NForumsImpl
