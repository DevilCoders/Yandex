#include "archivewriter.h"

#include <util/charset/wide.h>
#include <kernel/xpathmarker/utils/debug.h>

namespace NHtmlXPath {

TArchiveZoneWriter::TArchiveZoneWriter(IDocumentDataInserter& inserter)
    : Inserter(inserter)
{}

void TArchiveZoneWriter::FinishWriting() {
}

bool TArchiveZoneWriter::CanWrite(EExportType zoneType) {
    return (zoneType & ET_ARCHIVE_ZONES);
}

void TArchiveZoneWriter::WriteZone(const TZone& zone) {
    XPATHMARKER_INFO("Storing zone " << zone.Name << " to archive")
    Inserter.StoreZone(zone.Name.data(), zone.StartPosition, zone.EndPosition, true);
    for (size_t i = 0; i < zone.Attributes.size(); ++i) {
        const TAttribute& attribute = zone.Attributes[i];

        XPATHMARKER_INFO("Storing attribute " << attribute.Name << " with value = '" << attribute.Value << "' to archive")
        TUtf16String value = UTF8ToWide(attribute.Value);
        Inserter.StoreArchiveZoneAttr(attribute.Name.data(), value.data(), value.size(), attribute.Position);
    }
}

} // namespace NHtmlXPath

