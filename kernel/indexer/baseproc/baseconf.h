#pragma once

#include "indexconf.h"
#include "archconf.h"
#include "groupconf.h"

struct INDEX_CONFIG;

struct TBaseConfig
    : public TIndexProcessorConfig
    , public TArchiveProcessorConfig
    , public TGroupProcessorConfig
{
    TString XmlParserConf;
    TBaseConfig(const INDEX_CONFIG* config = nullptr);
    void CheckConfigsExists() const;
    void ResolveWorkDir();
};
