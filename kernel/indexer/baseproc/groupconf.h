#pragma once

#include <kernel/groupattrs/docsattrsdata.h>
#include <kernel/groupattrs/docsattrswriter.h>

#include <util/generic/string.h>

struct TGroupProcessorConfig {
    TString Indexaa;
    TString Groups;
    TString OldAttrPath;
    TString NewAttrPath;
    NGroupingAttrs::TVersion IndexaaVersion;
    bool UseOldC2N;
    TGroupProcessorConfig()
        : IndexaaVersion(NGroupingAttrs::TDocsAttrsWriter::DefaultVersion)
        , UseOldC2N(false)
    {}
};
