#pragma once

#include <util/generic/string.h>
#include <util/stream/input.h>

namespace NReMorph {

class TFileParseOptions {
public:
    TString RulesPath;
    TString BaseDirPath;

public:
    explicit TFileParseOptions(const TString& rulesPath);

protected:
    explicit TFileParseOptions();
};

class TStreamParseOptions: public TFileParseOptions {
public:
    IInputStream& Input;
    bool Inline;

public:
    explicit TStreamParseOptions(IInputStream& input);
    explicit TStreamParseOptions(IInputStream& input, const TString& rulesPath);
};

} // NReMorph
