#pragma once

#include <library/cpp/json/json_value.h>

#include <util/generic/ptr.h>

namespace NCoolBot {

////////////////////////////////////////////////////////////////////////////////

class TAutoTeamlead
{
public:
    TAutoTeamlead(const TString& stToken, const NJson::TJsonValue& config);
    ~TAutoTeamlead();

public:
    bool SelectAnswer(
        const TString& login,
        const TString& message,
        const TString& chatId,
        TString* answer,
        TString* sticker);

private:
    struct TImpl;
    THolder<TImpl> Impl;
};

}   // namespace NCoolBot
