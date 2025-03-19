#pragma once

#include "zonewriter.h"

#include <kernel/xpathmarker/utils/debug.h>

#include <library/cpp/json/json_writer.h>

#include <util/stream/output.h>

namespace NHtmlXPath {

using NJson::TJsonWriter;

class TJsonZoneWriter : public IZoneWriter {
public:
    TJsonZoneWriter(IOutputStream& output);
    ~TJsonZoneWriter() override;

    void WriteZone(const TZone& zone) override;
    bool CanWrite(EExportType zoneType) override;
    void FinishWriting() override;

private:
    void WriteAttribute(const TZone& zone, const TAttribute& attribute);

private:
    TJsonWriter JsonWriter;
    TString LastZoneName;
    bool NeedToReopenAttrMap;

#if XPATHMARKER_LOG_LEVEL >= XPATHMARKER_LOG_LEVEL_DEBUG
    static const bool IS_PRETTY_OUTPUT = true;
#else
    static const bool IS_PRETTY_OUTPUT = false;
#endif
};

} // namespace NHtmlXPath

