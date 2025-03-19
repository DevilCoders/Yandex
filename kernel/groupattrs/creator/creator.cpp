#include "creator.h"
#include "mergeattrs.h"

#include <kernel/groupattrs/mutdocattrs.h>

#include <util/system/yassert.h>
#include <ctype.h>

#include <library/cpp/deprecated/fgood/fgood.h>
#include <util/folder/dirut.h>
#include <util/string/cast.h>
#include <util/string/printf.h>

namespace NGroupingAttrs {

static void ParseFilename(const TString& fullname, TString* path, TString* file) {
    size_t slash = fullname.rfind(LOCSLASH_C);
    if (slash != TString::npos) {
        *path = TString(fullname.data(), 0, slash);
        *file = TString(fullname.data(), slash + 1, fullname.size() - slash - 1);
    } else {
        *path = ".";
        *file = fullname;
    }
}


static TString PortionName(const TString& path, const TString& name, ui32 number) {
    return Sprintf("%s%s%u.%s", path.data(), LOCSLASH_S, number, name.data());
}

bool TCreator::HasOpenPortion() const {
    return Out.Get() && Writer.Get();
}

void TCreator::OpenPortion() {
    const TString name = PortionName(Path, Filename, PortionsCount);
    Out.Reset(new TFixedBufferFileOutput(name));
    Writer.Reset(new TDocsAttrsWriter(Config_, PortionVersion, TConfig::Index, Path, *Out, (ToString(PortionsCount)+"."+Filename).c_str()));
}

void TCreator::RemovePortions() {
    for (ui32 i = 0; i < PortionsCount; ++i) {
        NFs::Remove(PortionName(Path, Filename, i));
    }
}

void TCreator::MakePortion() {
    if (!HasOpenPortion()) {
        return;
    }

    Writer->Close();
    Out->Finish();
    Writer.Destroy();
    Out.Destroy();

    ++PortionsCount;
}

void TCreator::Init(const TString& grconf, const TString& filename) {
    Config_.InitFromStringWithTypes(grconf.data());
    ParseFilename(filename, &Path, &Filename);

    IsOpen = true;
}

void TCreator::InitFromFile(const TString& grconffilename, const TString& filename) {
    Config_.InitFromFileWithBytes(grconffilename.data());
    ParseFilename(filename, &Path, &Filename);

    IsOpen = true;
}

void TCreator::AddDoc(TMutableDocAttrs& docAttrs) {
    Y_ASSERT(Config() == docAttrs.Config());

    if (!HasOpenPortion()) {
        OpenPortion();
    }

    Writer->Write(docAttrs);
}

void TCreator::Close() {
    if (!IsOpen) {
        return;
    }

    MakePortion();

    TVector<TString> portions;
    for (ui32 i = 0; i < PortionsCount; ++i) {
        portions.push_back(PortionName(Path, Filename, i));
    }

    MergeAttrsPortion(portions, "", Sprintf("%s/%s", Path.data(), Filename.data()), nullptr, ResultWriteMode, ResultVersion);
    RemovePortions();

    IsOpen = false;
}

void TCreator::SaveC2N(const TString& prefix) const {
    MetaCreator->SaveC2N(prefix);
}

void TCreator::LoadC2N(const TString& prefix) {
    MetaCreator->LoadC2N(Config(), prefix);
}

TCateg TCreator::ConvertAttrValue(const char* attrname, const char* value) {
    if (!Config().HasAttr(attrname)) {
        ythrow yexception() << "Attribute " << attrname << " is not configured";
    }

    return MetaCreator->AddCateg(attrname, value);
}

}
