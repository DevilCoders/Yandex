#pragma once

#include <kernel/common_server/library/geometry/coord.h>
#include <kernel/common_server/library/json/cast.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>

namespace NGeoJson {
    class TGeometry {
    public:
        enum EType {
            LineString,
            Polygon,
        };
        using TCoordinates = TVector<TGeoCoord>;
        using TMultiCoordinates = TVector<TCoordinates>;

    public:
        TGeometry(EType type = Polygon)
            : Type(type)
        {
        }

        const TCoordinates& GetCoordinates() const {
            return MultiCoordinates.size() ? MultiCoordinates[0] : Default<TCoordinates>();
        }
        TCoordinates& GetCoordinates() {
            if (MultiCoordinates.empty()) {
                MultiCoordinates.emplace_back();
            }
            return MultiCoordinates[0];
        }
        EType GetType() const {
            return Type;
        }

        NJson::TJsonValue ToJson() const;

    private:
        TMultiCoordinates MultiCoordinates;
        EType Type;
    };

    class TFeature {
    public:
        using TProperties = TMap<TString, NJson::TJsonValue>;

    public:
        ui64 GetId() const {
            return Id;
        }
        void SetId(ui64 value) {
            Id = value;
        }

        const TGeometry* GetGeometry() const {
            return Geometry.Get();
        }
        TGeometry& MutableGeometry() {
            return Geometry.GetRef();
        }
        template <class T>
        void SetGeometry(T&& value) {
            Geometry = std::move(value);
        }

        const TProperties& GetProperties() const {
            return Properties;
        }
        TProperties& GetProperties() {
            return Properties;
        }

        NJson::TJsonValue ToJson() const;

    private:
        ui64 Id = 0;
        TMaybe<TGeometry> Geometry;
        TProperties Properties;
    };

    class TFeatureCollection {
    public:
        using TFeatures = TVector<TFeature>;
        using TMetadata = TMap<TString, NJson::TJsonValue>;

    public:
        const TFeatures& GetFeatures() const {
            return Features;
        }
        TFeatures& GetFeatures() {
            return Features;
        }

        const TMetadata& GetMetadata() const {
            return Metadata;
        }
        TMetadata& GetMetadata() {
            return Metadata;
        }

        NJson::TJsonValue ToJson() const;

    private:
        TFeatures Features;
        TMetadata Metadata;
    };
}
