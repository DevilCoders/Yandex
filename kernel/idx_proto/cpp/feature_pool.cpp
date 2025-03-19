#include "feature_pool.h"

#include <util/string/split.h>
#include <util/stream/str.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>

void FeaturesTSVLineToProto(const TStringBuf& featuresLine, NFeaturePool::TLine& line) {
    TVector<TStringBuf> columns;
    StringSplitter(featuresLine.begin(), featuresLine.end()).Split('\t').AddTo(&columns);
    Y_ENSURE(columns.size() >= 4, "Invalid field count in feature line. ");

    line.SetRequestId(FromString<ui32>(columns[0]));
    line.SetRating(FromString<float>(columns[1]));
    line.SetMainRobotUrl(ToString(columns[2]));
    line.SetGrouping(ToString(columns[3]));
    line.SetFiltrationType(NFeaturePool::FT_NOT_FILTERED);

    const size_t FFI = 4;
    for (size_t i = FFI; i < columns.size(); ++i) {
        line.AddFeature(FromString<float>(columns[i]));
    }
}

void ProtoToFeaturesTSVLine(const NFeaturePool::TLine& line, TString& featuresLine, bool addRequestId) {
    const TStringBuf t = "\t";
    featuresLine.clear();
    TStringOutput poolBuilder(featuresLine);

    if (addRequestId) {
        poolBuilder << line.GetRequestId() << t;// column 0
    }

    poolBuilder     // column 0 may be skipped
                << line.GetRating() << t       // column 1
                << line.GetMainRobotUrl() << t // column 2
                << line.GetGrouping();         // column 3

    for (size_t i = 0; i != line.FeatureSize(); ++i) {
        poolBuilder << t << line.GetFeature(i);
    }
}
