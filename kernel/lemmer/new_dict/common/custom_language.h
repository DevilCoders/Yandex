#pragma once

#include "new_language.h"

#include <util/generic/string.h>
#include <util/memory/blob.h>

class TCustomLanguage: public TNewLanguage {
public:
    TCustomLanguage(ELanguage lang, const TString& path)
        : TNewLanguage(lang, true)
        , Data(TBlob::PrechargedFromFile(path))
    {
        Init(Data);
    }

protected:
    TBlob Data;
};
