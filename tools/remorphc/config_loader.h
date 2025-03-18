#pragma once

#include "config.h"
#include "gazetteer_pool.h"
#include "unit_config.h"
#include "var_replacer.h"

#include <kernel/remorph/common/source.h>

#include <util/folder/path.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>

#include <google/protobuf/text_format.h>

namespace NRemorphCompiler {

const char* GetDefaultConfigName();

struct TConfigLoadingError: public yexception {
    TString Path;
    NReMorph::TSourceLocation Location;

    TConfigLoadingError(const TString& path)
        : Path(path)
        , Location(Path)
    {
        *this << Path << ": ";
    }

    TConfigLoadingError(const TString& path, const NReMorph::TSourcePos& pos)
        : Path(path)
        , Location(Path, pos)
    {
        *this << Location << ": ";
    }
};

class TConfigLoader {
private:
    NProtoBuf::TextFormat::Parser PbParser;
    TConfig& Config;
    TVarReplacer VarReplacer;
    TGazetteerPoolPtr GazetteerPool;
    IOutputStream* Log;

public:
    TConfigLoader(TConfig& config, const TVars& vars, IOutputStream* log = nullptr);

    void Load(const TString& path, bool recursive);

private:
    void Load(const TFsPath& configPath);
    TString& ReplaceVars(TString& str, const TVars& runtimeVars) const;
};

} // NRemorphCompiler
