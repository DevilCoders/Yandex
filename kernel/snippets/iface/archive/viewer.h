#pragma once

#include <util/generic/vector.h>

#include <util/system/defaults.h>

#include <util/stream/mem.h>

class TArchiveMarkupZones;

namespace NSnippets {
    class TUnpacker;
    namespace NSegments {
        class TSegmentsInfo;
    }
    class IArchiveViewer {
    public:
        virtual void OnUnpacker(TUnpacker*) {
        }
        virtual void OnEnd() {
        }
        virtual void OnHitBase(int) {
        }
        virtual void OnWeightZones(TMemoryInput*) {
        }
        virtual void OnMarkup(const TArchiveMarkupZones&) {
        }
        virtual void OnHitsAndSegments(const TVector<ui16>&, const NSegments::TSegmentsInfo*) {
        }
        virtual void OnBeforeSents() {
        }
        virtual ~IArchiveViewer() = default;
    };
}
