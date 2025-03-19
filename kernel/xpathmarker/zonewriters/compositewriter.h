#pragma once

#include "zonewriter.h"

#include <util/generic/vector.h>
#include <util/generic/ptr.h>

namespace NHtmlXPath {

class TCompositeZoneWriter : public IZoneWriter {
public:
    void AddWriter(TSimpleSharedPtr<IZoneWriter> writer) {
       Writers.push_back(writer);
    }

    bool CanWrite(EExportType zoneType) override {
        for (size_t i = 0; i < Writers.size(); ++i) {
            if (Writers[i]->CanWrite(zoneType)) {
                return true;
            }
        }
        return false;
    }

    void WriteZone(const TZone& zone) override {
        for (size_t i = 0; i < Writers.size(); ++i) {
            if (Writers[i]->CanWrite(zone.ExportType)) {
                Writers[i]->WriteZone(zone);
            }
        }
    }

    void FinishWriting() override {
        for (size_t i = 0; i < Writers.size(); ++i) {
            Writers[i]->FinishWriting();
        }
    }

    ~TCompositeZoneWriter() override {};
private:
    TVector< TSimpleSharedPtr<IZoneWriter> > Writers;
};

} // namespace NHtmlXPath

