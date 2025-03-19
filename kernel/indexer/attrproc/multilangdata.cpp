#include <util/charset/wide.h>
#include <kernel/indexer/face/inserter.h>
#include <yweb/robot/dbscheeme/mergecfg.h>
#include <kernel/langregion/langregion.h>

#include "attrconf.h"
#include "multilangdata.h"

TMultilanguageDocData::TMultilanguageDocData(const TAttrProcessorConfig* cfg)
    : MultilanguageDocsFile("multilanguagedocsfile", dbcfg::fbufsize, 0)
{
    char name[PATH_MAX];
    sprintf(name, TString::Join("%s/", dbcfg::fname_clsmultilanguagedoc).data(), cfg->HomeDir.data(), cfg->Cluster);
    MultilanguageDocsFile.Open(name);
    MultilanguageDocsFile.Next();
}

const TDocGroupRec* TMultilanguageDocData::MoveToDoc(ui32 docId) {
    while (MultilanguageDocsFile.Current() && MultilanguageDocsFile.Current()->DocId < docId)
        MultilanguageDocsFile.Next();
    return MultilanguageDocsFile.Current();
}

void TMultilanguageDocData::StoreMultilanguageAttr(IDocumentDataInserter& inserter, ui32 docId) {
    const TDocGroupRec* rec = MoveToDoc(docId);
    if (rec && rec->DocId == docId) {
        const TUtf16String val(UTF8ToWide(LangRegion2Str(static_cast<ELangRegion>(rec->GroupId))));
        inserter.StoreLiteralAttr("multilanguage", val.data(), val.size(), 0);
    }
}

