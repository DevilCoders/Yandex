#pragma once

#include <library/cpp/reverse_geocoder/core/location.h>
#include <library/cpp/reverse_geocoder/core/point.h>
#include <library/cpp/reverse_geocoder/proto/region.pb.h>

#include <util/generic/vector.h>

namespace NReverseGeocoder {
    namespace NGenerator {
        namespace {
            inline double Lon(const TLocation& l) {
                return l.Lon;
            }

            inline double Lat(const TLocation& l) {
                return l.Lat;
            }

            inline double Lon(const NProto::TLocation& l) {
                return l.GetLon();
            }

            inline double Lat(const NProto::TLocation& l) {
                return l.GetLat();
            }

            template <typename Ta, typename Tb>
            bool IsEqualLocations(Ta const& a, Tb const& b) {
                TLocation a1(Lon(a), Lat(a));
                TLocation b1(Lon(b), Lat(b));
                return TPoint(a1) == TPoint(b1);
            }

            template <typename TLocation>
            bool OnOneLine(const TLocation& a0, const TLocation& b0, const TLocation& c0) {
                const TPoint a(a0);
                const TPoint b(b0);
                const TPoint c(c0);
                return (b - a).Cross(c - b) == 0;
            }

        }

        class TLocationsConverter {
        public:
            template <typename locationsT, typename TCallback>
            void Each(locationsT const& rawLocations, TCallback callback) {
                TVector<TLocation> locations;
                locations.reserve(rawLocations.size());

                for (auto const& l : rawLocations) {
                    if (locations.empty() || !IsEqualLocations(locations.back(), l))
                        locations.push_back(TLocation(Lon(l), Lat(l)));

                    if (locations.size() >= 3) {
                        const TLocation& a = locations[locations.size() - 3];
                        const TLocation& b = locations[locations.size() - 2];
                        const TLocation& c = locations.back();
                        if (OnOneLine(a, b, c)) {
                            locations[locations.size() - 2] = locations.back();
                            locations.pop_back();
                        }
                    }

                    if (locations.size() >= 3 && IsEqualLocations(locations.front(), locations.back())) {
                        locations.pop_back();
                        if (locations.size() >= 3)
                            callback(locations);
                        locations.clear();
                    }
                }

                if (locations.size() >= 3)
                    callback(locations);
            }
        };

    }
}
