#pragma once

#include <library/cpp/reverse_geocoder/open_street_map/proto/open_street_map.pb.h>

#include <util/stream/input.h>
#include <util/system/mutex.h>
#include <util/system/guard.h>

namespace NReverseGeocoder {
    namespace NOpenStreetMap {
        class TReader {
        public:
            explicit TReader(IInputStream* inputStream)
                : InputStream_(inputStream)
            {
            }

            bool Read(NProto::TBlobHeader* header, NProto::TBlob* blob);

        private:
            IInputStream* InputStream_;
            TMutex Mutex_;
        };

    }
}
