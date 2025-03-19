#pragma once

#include "config.h"
#include "docsattrsdata.h"
#include "docsattrswriter.h"

#include <kernel/search_types/search_types.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>

class TFsPath;
class IOutputStream;

namespace NGroupingAttrs {

class TPacker {
private:
    typedef THashMap<TString, TCateg> TMaxValues;

private:
    TConfig Config;
    THolder<IDocsAttrsWriter> Writer;

    TMaxValues MaxValues;

private:
    void CalcMaxValue(const TString& filename);
    void CalcMaxValues(const TString& config);
    void InitConfig();
    void Merge(const TString& config);

public:
    void Pack(const TString& config, TVersion version, const TString& output);
    void PackToStream(const TString& config, TVersion version, const TString& output, const TFsPath& tmpDir);
};

}
