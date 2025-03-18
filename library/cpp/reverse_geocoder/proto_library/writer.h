#pragma once

#include <library/cpp/reverse_geocoder/proto/region.pb.h>
#include <util/stream/file.h>
#include <util/generic/vector.h>
#include <util/network/address.h>
#include <util/system/mutex.h>
#include <util/system/guard.h>

namespace NReverseGeocoder {
    namespace NProto {
        class TWriter {
        public:
            explicit TWriter(const char* path)
                : Mutex_()
                , OutputStream_(path)
            {
            }

            template <typename TMessage>
            void Write(const TMessage& message) {
                const ui32 byteSize = message.ByteSize();
                const ui32 totalSize = byteSize + sizeof(byteSize);

                TVector<char> buffer(totalSize);

                *((ui32*)buffer.data()) = htonl(byteSize);

                if (!message.SerializeToArray(buffer.data() + sizeof(byteSize), byteSize))
                    ythrow yexception() << "Unable to serialize message to array";

                TGuard<TMutex> Lock(Mutex_);
                OutputStream_.Write(buffer.data(), buffer.size());
            }

        private:
            TMutex Mutex_;
            TFixedBufferFileOutput OutputStream_;
        };

    }
}
