#include "jsonwriter.h"

#include <kernel/xpathmarker/utils/debug.h>

#include <util/string/cast.h>

namespace NHtmlXPath {

TJsonZoneWriter::TJsonZoneWriter(IOutputStream& output)
    : JsonWriter(&output, /*formatOutput = */ IS_PRETTY_OUTPUT)
    , NeedToReopenAttrMap(false)
{}

void TJsonZoneWriter::FinishWriting() {
    if (!LastZoneName.empty()) {
        JsonWriter.CloseMap();   // - zone attribute map
        JsonWriter.CloseArray(); // - zone occurence array
        JsonWriter.CloseMap();   // - zone map

        JsonWriter.Flush();
        LastZoneName = "";
    }
}

TJsonZoneWriter::~TJsonZoneWriter()
{}

bool TJsonZoneWriter::CanWrite(EExportType zoneType) {
    return (zoneType & ET_JSON);
}

void TJsonZoneWriter::WriteZone(const TZone& zone) {
    XPATHMARKER_INFO("Writing zone " << zone.Name << " to json")
    if (zone.Name == LastZoneName) {
        NeedToReopenAttrMap = true;
    }
    for (size_t i = 0; i < zone.Attributes.size(); ++i) {
        XPATHMARKER_INFO("Writing attribute " << zone.Attributes[i].Name << " with value '" << zone.Attributes[i].Value << "' to json")
        WriteAttribute(zone, zone.Attributes[i]);
    }
}

void TJsonZoneWriter::WriteAttribute(const TZone& zone, const TAttribute& attribute) {
    bool needToReopenOccurArray = false;

    if (LastZoneName.empty()) {
        JsonWriter.OpenMap();        // + zone map
    }
    if (LastZoneName != zone.Name) {
        needToReopenOccurArray = true;
        NeedToReopenAttrMap = false;
    }
    if (NeedToReopenAttrMap) {
        JsonWriter.CloseMap();       // - zone attribute map
    }
    if (needToReopenOccurArray) {
        if (!LastZoneName.empty()) {
            JsonWriter.CloseArray(); // - zone occurence array
        }
        JsonWriter.Write(zone.Name);
        JsonWriter.OpenArray();      // + zone occurence array
        NeedToReopenAttrMap = true;
    }
    if (NeedToReopenAttrMap) {
        JsonWriter.OpenMap();        // + zone attribute map
    }

    switch (attribute.Type) {
        case AT_INTEGER:
            JsonWriter.Write(attribute.Name, FromString<i64>(attribute.Value));
            break;
        case AT_BOOLEAN:
            JsonWriter.Write(attribute.Name, (bool) (attribute.Value == "1"));
            break;
        default:
            JsonWriter.Write(attribute.Name, attribute.Value);
    }

    LastZoneName = zone.Name;
    NeedToReopenAttrMap = false;
}

} // namespace NHtmlXPath

