#pragma once

#include <kernel/groupattrs/utils.h>
#include <kernel/groupattrs/printer.h>
#include <kernel/groupattrs/config.h>
#include <kernel/groupattrs/metainfo.h>


#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/folder/path.h>

#include <tools/idx_print/utils/options.h>

/*
    Print .c2n{.wad} files
*/
void PrintCategToName(const TIdxPrintOptions& options) {
    Y_ENSURE(options.IndexPath.EndsWith(".wad"));
    NGroupingAttrs::TOffroadWadStaticClassNames wadCategToName(/*lockMemory=*/false);
    wadCategToName.LoadFromPath(options.IndexPath, /*useRev=*/false);
    if (options.DocIds.empty()) {
        for (size_t i = 1; i <= wadCategToName.CategCount(); ++i) {
            TString name = ToString(wadCategToName.Categ2Name(i));
            Y_ENSURE(!name.empty());
            Cout << i << '\t' << name << '\n';
        }
    } else {
        for (const ui32 id : options.DocIds) {
            TString name = ToString(wadCategToName.Categ2Name(id));
            Y_ENSURE(!name.empty());
            Cout << id << '\t' << name << '\n';
        }
    }
}

/*
    Print indexaa.wad with the provided metainfo {h,d}.c2n{.wad}
*/


class TCategToNameSearcher {
    using TKeySearcher = NOffroad::TFatSearcher<ui32, NOffroad::TNullSerializer>;
public:
    TCategToNameSearcher(const TString& path) {
        Y_ENSURE(NFs::Exists(path));

        Wad_ = NDoom::IWad::Open(path, false);

        FatCategToName_ = Wad_->LoadGlobalLump(NDoom::TWadLumpId(NDoom::CategToNameIndexType, NDoom::EWadLumpRole::KeyFat));
        FatSubCategToName_ = Wad_->LoadGlobalLump(NDoom::TWadLumpId(NDoom::CategToNameIndexType, NDoom::EWadLumpRole::KeyIdx));
        KeySearcher_.Reset(FatCategToName_, FatSubCategToName_);
    }

    size_t CategCount() const{
        return KeySearcher_.Size() - 1;
    }

    const char* Categ2Name(TCateg aClass) const {
        if (aClass < 0 || static_cast<ui64>(aClass) > this->CategCount()) {
            return "";
        }
        return KeySearcher_.ReadKey(aClass).data();
    }

private:
    THolder<NDoom::IWad> Wad_;
    TBlob FatCategToName_;
    TBlob FatSubCategToName_;
    TKeySearcher KeySearcher_;
};



void PrintDocAttrsIndex(const TIdxPrintOptions& options) {
    Y_ENSURE(options.PrintHits, "Nothing to print, use -H or --print-hits to print hits.");

    NDoom::TWadLumpId id(NDoom::EWadIndexType::DocAttrsIndexType, NDoom::EWadLumpRole::Struct);
    NDoom::TOffroadDocAttrsWadIo::TReader reader(options.IndexPath);
    // read and print config
    NGroupingAttrs::TConfig config;

    TBlob blob = reader.LoadGlobalLump(id);

    config.InitFromStringWithTypes(TString(blob.AsCharPtr(), blob.Size()));
    for (ui32 attrnum = 0; attrnum < config.AttrCount(); ++attrnum) {
        Cout << config.AttrName(attrnum) << '\t' << NGroupingAttrs::TPrinter::Type(config.AttrType(attrnum)) << '\n';
    }

    const TString hostPath = TFsPath(options.IndexPath).Parent() / "h.c2n.wad";
    const TString domainPath = TFsPath(options.IndexPath).Parent() / "d.c2n.wad";

    TCategToNameSearcher hosts(hostPath);
    TCategToNameSearcher domains(domainPath);

    NDoom::TOffroadDocAttrsWadIo::TReader::THit hit;
    ui32 docId;

    while (reader.ReadDoc(&docId)) {
        if (!options.DocIds.empty() && !options.DocIds.contains(docId))
            continue;

        while (reader.ReadHit(&hit)) {
            TString attrName = config.AttrName(hit.AttrNum());
            Cout << docId << '\t' << attrName << '\t' << hit.Categ() << '\t';
            if (attrName == "h") {
                Y_ENSURE(hit.Categ() <= hosts.CategCount());
                Cout << hosts.Categ2Name(hit.Categ());
            }

            if (attrName == "d") {
                Y_ENSURE(hit.Categ() <= domains.CategCount());
                Cout << domains.Categ2Name(hit.Categ());
            }

            Cout << "\n";
        }
    }
}
