#include <library/cpp/reverse_geocoder/library/log.h>
#include <library/cpp/reverse_geocoder/library/stop_watch.h>
#include <library/cpp/reverse_geocoder/proto_library/reader.h>

void NReverseGeocoder::NProto::TReader::GenerateIndex() {
    if (!Index_.empty()) {
        LogWarning("Index already generated!");
        return;
    }

    LogDebug("Generating proto reader index...");

    TStopWatch stopWatch;
    stopWatch.Run();

    EachWithPtr([&](const char* ptr, const NProto::TRegion& region) {
        if (Index_.find(region.GetRegionId()) != Index_.end())
            LogWarning("Region %lu already exists in index", region.GetRegionId());
        Index_[region.GetRegionId()] = ptr - ((const char*)MemFile_.Ptr());
    });

    float const seconds = stopWatch.Get();
    LogDebug("Proto reader index generated in %.3f seconds (%.3f minutes)",
             seconds, seconds / 60.0);
}
