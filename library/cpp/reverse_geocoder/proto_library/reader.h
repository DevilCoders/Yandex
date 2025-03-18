#pragma once

#include <library/cpp/reverse_geocoder/core/common.h>
#include <library/cpp/reverse_geocoder/proto/region.pb.h>
#include <util/generic/hash_set.h>
#include <util/system/filemap.h>
#include <util/system/unaligned_mem.h>
#include <util/network/address.h>

namespace NReverseGeocoder {
    namespace NProto {
        class TReader: public TNonCopyable {
        public:
            explicit TReader(const char* path)
                : Path_(path)
                , MemFile_(path)
                , Index_()
            {
                MemFile_.Map(0, MemFile_.Length());
            }

            template <typename TCallback>
            void Each(TCallback callback) {
                EachWithPtr([&](const char*, const NProto::TRegion& region) {
                    callback(region);
                });
            }

            template <typename TCallback>
            bool Region(TGeoId geoId, TCallback callback) {
                if (Index_.empty()) {
                    Each([&](const NProto::TRegion& region) {
                        if (region.GetRegionId() == geoId)
                            callback(region);
                    });

                } else {
                    if (Index_.find(geoId) == Index_.end())
                        return false;

                    const char* ptr = ((const char*)MemFile_.Ptr()) + Index_[geoId];
                    const ui32 byteSize = ntohl(ReadUnaligned<ui32>(ptr));

                    NProto::TRegion region;
                    if (!region.ParseFromArray(ptr + sizeof(byteSize), byteSize))
                        ythrow yexception() << "Unable parse region from array";

                    callback(region);
                }

                return true;
            }

            void GenerateIndex();

        private:
            template <typename TCallback>
            void EachWithPtr(TCallback callback) {
                const char* begin = (const char*)MemFile_.Ptr();
                const char* end = begin + MemFile_.MappedSize();

                const char* ptr = begin;
                NProto::TRegion region;

                while (ptr < end) {
                    const ui32 byteSize = ntohl(ReadUnaligned<ui32>(ptr));
                    if (!region.ParseFromArray(ptr + sizeof(byteSize), byteSize))
                        ythrow yexception() << "Unable parse region from array; byteSize " << byteSize << "(" << Path_ << ")";
                    callback(ptr, (const NProto::TRegion&)region);
                    ptr += byteSize + sizeof(byteSize);
                }
            }

            const char* Path_;
            TFileMap MemFile_;
            THashMap<TGeoId, size_t> Index_;
        };

    }
}
