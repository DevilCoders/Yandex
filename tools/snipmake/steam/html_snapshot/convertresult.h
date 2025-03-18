#pragma once

#include <util/generic/string.h>

namespace NSteam
{

struct TConvertResult
{
    bool Done;
    TString Data;
    TString ErrorMessage;

    TConvertResult()
        : Done(false)
    {
    }
};

}

