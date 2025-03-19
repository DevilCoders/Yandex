#include "value.h"

NJson::TJsonValue NGeoJson::TGeometry::ToJson() const {
    NJson::TJsonValue result;
    result["type"] = ToString(Type);
    for (auto&& coordinates : MultiCoordinates) {
        NJson::TJsonValue& cc = Type == Polygon ? result["coordinates"].AppendValue(NJson::JSON_ARRAY) : result["coordinates"];
        for (auto&& coordinate : coordinates) {
            NJson::TJsonValue& c = cc.AppendValue(NJson::JSON_ARRAY);
            c.AppendValue(coordinate.X);
            c.AppendValue(coordinate.Y);
        }
    }
    return result;
}

template <>
NJson::TJsonValue NJson::ToJson<NGeoJson::TGeometry>(const NGeoJson::TGeometry& object) {
    return object.ToJson();
}

template <>
bool NJson::TryFromJson<NGeoJson::TGeometry>(const TJsonValue& value, NGeoJson::TGeometry& result) {
    NGeoJson::TGeometry::EType type;
    if (!TryFromString(value["type"].GetStringRobust(), type)) {
        return false;
    }
    NGeoJson::TGeometry object(type);
    const NJson::TJsonValue& coordinates = value["coordinates"];
    if (!coordinates.IsArray()) {
        return false;
    }
    const NJson::TJsonValue* points = nullptr;
    switch (type) {
    case NGeoJson::TGeometry::LineString:
        points = &coordinates;
        break;
    case NGeoJson::TGeometry::Polygon: {
        const auto& arr = coordinates.GetArray();
        if (arr.empty()) {
            return false;
        }
        points = &coordinates.GetArray().front();
        break;
    }
    }
    if (!points) {
        return false;
    }
    if (!points->IsArray()) {
        return false;
    }
    for (auto&& c : points->GetArray()) {
        if (!c.IsArray()) {
            return false;
        }
        const auto& arr = c.GetArray();
        if (arr.size() < 2) {
            return false;
        }
        double latitude = 0;
        double longitude = 0;
        if (!arr[0].GetDouble(&longitude)) {
            return false;
        }
        if (!arr[1].GetDouble(&latitude)) {
            return false;
        }
        object.GetCoordinates().emplace_back(longitude, latitude);
    }
    result = std::move(object);
    return true;
}

NJson::TJsonValue NGeoJson::TFeature::ToJson() const {
    NJson::TJsonValue result;
    result["type"] = "Feature";
    result["id"] = Id;
    if (Geometry) {
        result["geometry"] = Geometry->ToJson();
    }
    for (auto&&[key, value] : Properties) {
        result["properties"][key] = value;
    }
    return result;
}

template <>
NJson::TJsonValue NJson::ToJson<NGeoJson::TFeature>(const NGeoJson::TFeature& object) {
    return object.ToJson();
}

template <>
bool NJson::TryFromJson<NGeoJson::TFeature>(const TJsonValue& value, NGeoJson::TFeature& result) {
    if (value["type"].GetStringRobust() != "Feature") {
        return false;
    }

    NGeoJson::TFeature object;

    unsigned long long id;
    if (!value["id"].GetUInteger(&id)) {
        return false;
    }
    object.SetId(id);

    const auto& geometry = value["geometry"];
    if (geometry.IsDefined()) {
        NGeoJson::TGeometry g;
        if (!TryFromJson(geometry, g)) {
            return false;
        }
        object.SetGeometry(std::move(g));
    }

    const auto& properties = value["properties"];
    if (properties.IsDefined()) {
        if (!properties.IsMap()) {
            return false;
        }
        for (auto&&[key, value] : properties.GetMap()) {
            object.GetProperties().emplace(key, value);
        }
    }

    result = std::move(object);
    return true;
}

NJson::TJsonValue NGeoJson::TFeatureCollection::ToJson() const {
    NJson::TJsonValue result;
    result["type"] = "FeatureCollection";
    for (auto&& feature : Features) {
        result["features"].AppendValue(feature.ToJson());
    }
    for (auto&&[key, value] : Metadata) {
        result["metadata"][key] = value;
    }
    return result;
}

template <>
NJson::TJsonValue NJson::ToJson<NGeoJson::TFeatureCollection>(const NGeoJson::TFeatureCollection& object) {
    return object.ToJson();
}

template <>
bool NJson::TryFromJson<NGeoJson::TFeatureCollection>(const TJsonValue& value, NGeoJson::TFeatureCollection& result) {
    if (value["type"].GetStringRobust() != "FeatureCollection") {
        return false;
    }

    NGeoJson::TFeatureCollection object;

    const auto& features = value["features"];
    if (features.IsDefined()) {
        if (!features.IsArray()) {
            return false;
        }
        for (auto&& feature : features.GetArray()) {
            NGeoJson::TFeature f;
            if (!TryFromJson(feature, f)) {
                return false;
            }
            object.GetFeatures().push_back(std::move(f));
        }
    }

    const auto& metadata = value["metadata"];
    if (metadata.IsDefined()) {
        if (!metadata.IsMap()) {
            return false;
        }
        for (auto&&[key, value] : metadata.GetMap()) {
            object.GetMetadata().emplace(key, value);
        }
    }

    result = std::move(object);
    return true;
}
