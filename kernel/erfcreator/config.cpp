#include "config.h"

#include "paths.h"

#include <library/cpp/getopt/opt2.h>

#include <util/string/cast.h>

IRemapReader* GetInvRemap(const TErfCreateConfig& /*config*/) {
    return new TTrivialRemapReader();
}

IRemapReader* GetRemap(const TErfCreateConfig& /*config*/) {
    return new TTrivialRemapReader();
}

void TErfCreateConfig::ParseArgs(int argc, char* argv[]) {
    this->TErfCreateConfigCommon::ParseArgs(argc, argv);

    if (Patch) {
        CheckPatchFields();
    }
}

SDocErfInfo3::TFieldMask TErfCreateConfig::GetErfFieldMask() const {
    return SDocErfInfo3::TFieldMask::Parse(PatchFields);
}

SDocErf2Info::TFieldMask TErfCreateConfig::GetErf2FieldMask() const {
    return SDocErf2Info::TFieldMask::Parse(PatchFields);
}

THostErfInfo::TFieldMask TErfCreateConfig::GetHErfFieldMask() const {
    return THostErfInfo::TFieldMask::Parse(PatchFields);
}

TRegErfInfo::TFieldMask TErfCreateConfig::GetRegErfFieldMask() const {
    return TRegErfInfo::TFieldMask::Parse(PatchFields);
}

TRegHostErfInfo::TFieldMask TErfCreateConfig::GetRegHErfFieldMask() const {
    return TRegHostErfInfo::TFieldMask::Parse(PatchFields);
}

void TErfCreateConfig::CheckPatchFields() const
{
    bool incompatible = false;

    SDocErfInfo3::TFieldMask erfFieldMask = GetErfFieldMask();

    size_t nRootFlds = erfFieldMask.nRoot;
    nRootFlds += erfFieldMask.nNews;
    nRootFlds += erfFieldMask.IsMainPage;
    if ((nRootFlds != 0) && (nRootFlds != 3))
        incompatible = true;

    if (erfFieldMask.IsUnreachable != erfFieldMask.Hops)
        incompatible = true;

    if (incompatible) {
        ythrow yexception() << "Some mutually dependent fields haven't been specified together";
    }
}
