#pragma once

#include "zonewriter.h"

#include <kernel/indexer/face/inserter.h>

namespace NHtmlXPath {

class TArchiveZoneWriter : public IZoneWriter {
public:
    explicit TArchiveZoneWriter(IDocumentDataInserter& inserter);

    void WriteZone(const TZone& zone) override;
    void FinishWriting() override;
    bool CanWrite(EExportType zoneType) override;

private:
    IDocumentDataInserter& Inserter;
};

} // namespace NHtmlXPath

