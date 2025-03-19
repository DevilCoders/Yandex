#include "groupproc.h"
#include <kernel/groupattrs/creator/creator.h>
#include <kernel/groupattrs/mutdocattrs.h>
#include <kernel/indexer/face/inserter.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <library/cpp/html/face/parsface.h>
#include <util/string/split.h>
#include <util/generic/ylimits.h>

TGroupProcessor::TGroupProcessor(const TGroupProcessorConfig* cfg, NGroupingAttrs::IMetaInfoCreator* metaInfoCreator) {
    Creator.Reset(new NGroupingAttrs::TCreator(NGroupingAttrs::TConfig::Index, metaInfoCreator, cfg->IndexaaVersion));
    Creator->Init(cfg->Groups.data(), cfg->Indexaa.data());
    Config = &Creator->Config();
    if (cfg->UseOldC2N)
        Creator->LoadC2N(cfg->OldAttrPath);
    NewAttrPath = cfg->NewAttrPath;
}

TGroupProcessor::~TGroupProcessor() {
}

static TCateg parseprop(const char*& ptr) {
    while (ptr && *ptr && !isdigit(*ptr))
        ptr++;
    if (!ptr || !*ptr)
        return END_CATEG;
    char *stop;
    TCateg prop = strtoll(ptr, &stop, 10);
    if (prop == Max<TCateg>() || prop < 0)
        ythrow yexception() << "Attribute out of range (" <<  ptr << ")";
    ptr = stop;
    return prop;
}

void TGroupProcessor::AddDocAttrs(TFullDocAttrs& extAttrs, const IParsedDocProperties* ps)
{
    if (!ps) {
        return;
    }

    for (ui32 attrnum = 0; attrnum < Config->AttrCount(); ++attrnum) {
        const char* prop = nullptr;
        const char* name = Config->AttrName(attrnum);
        if (ps->GetProperty(name, &prop) != 0)
            continue;

        if (Config->IsAttrNamed(attrnum)) {
            extAttrs.AddAttr(name, prop, TFullDocAttrs::AttrGrName);
        } else {
            TCateg val;
            const char* ptr = prop;
            while ((val = parseprop(ptr)) != END_CATEG) {
                extAttrs.AddAttr(name, ToString(val), TFullDocAttrs::AttrGrInt, Config->AttrType(attrnum));
            }
        }
    }
}

void TGroupProcessor::CommitDoc(const IParsedDocProperties* ps, ui32 docId, TFullDocAttrs& extAttrs, bool AddAttrsIntoExt) {
    NGroupingAttrs::TMutableDocAttrs da(*Config, docId);
    if (AddAttrsIntoExt)
        AddDocAttrs(extAttrs, ps); // add only the attrs declared in 'Group'-directive
    for (TFullDocAttrs::TConstIterator i = extAttrs.Begin(); i != extAttrs.End(); ++i) {
        if ((i->Type & (TFullDocAttrs::AttrGrName | TFullDocAttrs::AttrGrInt))
           && !da.Config().HasAttr(i->Name.data())) {
            Creator->MakePortion();

            NGroupingAttrs::TConfig newconfig = *Config;
            newconfig.AddAttr(i->Name.data(), i->SizeOfInt);
            da.ResetConfig(newconfig); //nb! resets Config also
        }

        if (i->Type & TFullDocAttrs::AttrGrName) {
            TCateg val = Creator->ConvertAttrValue(i->Name.data(), i->Value.data());
            da.SetAttr(i->Name.data(), val);
        }
        if (i->Type & TFullDocAttrs::AttrGrInt) {
            TCateg val;
            const char* ptr = i->Value.data();
            while ((val = parseprop(ptr)) != END_CATEG) {
                da.SetAttr(i->Name.data(), val);
            }
        }
    }

    Creator->AddDoc(da);
}

void TGroupProcessor::Term() {
    Creator->SaveC2N(NewAttrPath); // now we not support the reindexing here...
    Creator->Close();
}

TGroupDocumentAction::TGroupDocumentAction(const TGroupProcessorConfig* cfg)
: GroupingConfig(NGroupingAttrs::TConfig::Index)
{
    GroupingConfig.InitFromStringWithTypes(cfg->Groups.data());
}

void TGroupDocumentAction::OnDocProcessingStop(const IParsedDocProperties* pars, const TDocInfoEx*, IDocumentDataInserter* inserter, bool isFullProcessing) {
    if (!isFullProcessing)
        return;
    // parser properties to groups
    const char* prop = nullptr;
    for (ui32 i = 0; i < GroupingConfig.AttrCount(); ++i) {
        const char* infokey = GroupingConfig.AttrName(i);
        if (pars->GetProperty(infokey, &prop) == 0) {
            inserter->StoreGrpDocAttr(infokey, prop, !GroupingConfig.IsAttrNamed(i));
        }
    }
}
