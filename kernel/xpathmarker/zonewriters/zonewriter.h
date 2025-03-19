#pragma once

#include <kernel/xpathmarker/entities/zone.h>

namespace NHtmlXPath {

class IZoneWriter {
public:
    virtual bool CanWrite(EExportType zoneType) = 0;
    virtual void WriteZone(const TZone& zone) = 0;
    virtual void FinishWriting() = 0;
    virtual ~IZoneWriter() {};
};

} //namespace NHtmlXPath

