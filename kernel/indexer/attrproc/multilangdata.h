#pragma once

#include <library/cpp/microbdb/safeopen.h>
#include <yweb/robot/dbscheeme/baserecords.h>

struct TAttrProcessorConfig;
class IDocumentDataInserter;

class TMultilanguageDocData {
public:
    explicit TMultilanguageDocData(const TAttrProcessorConfig* cfg);

    const TDocGroupRec* MoveToDoc(ui32 docId);
    void StoreMultilanguageAttr(IDocumentDataInserter& inserter, ui32 docId);

private:
    TInDatFile<TDocGroupRec> MultilanguageDocsFile;
};

