#pragma once
#include <kernel/indexer/tfproc/date_recognizer.h>
#include "forums_fsm.h"

namespace NForumsImpl {

class TForumDateRecognizer
{
public:
    TForumDateRecognizer()
        : DateRecognizer(true)
        , DocDate(0)
        , NextDateIsRegistration(false)
    {
    }

    void SetDate(time_t date)
    {
        DocDate = date;
    }
    void Clear()
    {
        SpecialDate.Day = 0;
        SpecialDate.Month = 0;
        SpecialDate.Year = 0;
        SpecialRecognizer.InitFSM();
        RecognizedDate.Day = 0;
        RecognizedDate.Month = 0;
        RecognizedDate.Year = 0;
        NextDateIsRegistration = false;
        DateRecognizer.Clear();
    }
    void OnDateText(const wchar16* text, unsigned len);
    bool GetDate(TRecognizedDate* result)
    {
        if (SpecialDate.ToInt64()) {
            *result = SpecialDate;
            return true;
        }
        if (RecognizedDate.ToInt64()) {
            *result = RecognizedDate;
            return true;
        }
        return DateRecognizer.GetDate(result, true);
    }
private:
    TStreamDateRecognizer DateRecognizer;
    TSpecialDateRecognizer SpecialRecognizer;
    TRecognizedDate SpecialDate, RecognizedDate;
    time_t DocDate;
    bool NextDateIsRegistration;
};

} // namespace NForumsImpl
