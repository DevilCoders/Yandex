#pragma once

#include <kernel/erfcreator/common_config/common_config.h>
#include <util/generic/string.h>

#include <kernel/doc_remap/remap_reader.h>

#include <ysite/yandex/pure/pure.h>
#include <ysite/yandex/erf_format/erf_format.h>

struct TErfCreateConfig : public TErfCreateConfigCommon {
    void ParseArgs(int argc, char *argv[]);

    SDocErfInfo3::TFieldMask GetErfFieldMask() const;
    SDocErf2Info::TFieldMask GetErf2FieldMask() const;
    THostErfInfo::TFieldMask GetHErfFieldMask() const;
    TRegErfInfo::TFieldMask GetRegErfFieldMask() const;
    TRegHostErfInfo::TFieldMask GetRegHErfFieldMask() const;
    void CheckPatchFields() const;
};

class IRemapReader;
IRemapReader* GetRemap(const TErfCreateConfig& config);
IRemapReader* GetInvRemap(const TErfCreateConfig& config);
