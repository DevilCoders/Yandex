#include "parse_options.h"

#include <util/folder/path.h>
#include <util/generic/yexception.h>

namespace NReMorph {

TFileParseOptions::TFileParseOptions(const TString& rulesPath)
    : RulesPath(TFsPath(rulesPath).RealPath().c_str())
    , BaseDirPath(TFsPath(rulesPath).Parent().c_str())
{
    if (RulesPath.empty()) {
        throw yexception() << "Empty rules file path";
    }
}

TFileParseOptions::TFileParseOptions()
    : RulesPath("<inline>")
    , BaseDirPath()
{
}

TStreamParseOptions::TStreamParseOptions(IInputStream& input)
    : TFileParseOptions()
    , Input(input)
    , Inline(true)
{
}

TStreamParseOptions::TStreamParseOptions(IInputStream& input, const TString& rulesPath)
    : TFileParseOptions(rulesPath)
    , Input(input)
    , Inline(false)
{
}

} // NReMorph
