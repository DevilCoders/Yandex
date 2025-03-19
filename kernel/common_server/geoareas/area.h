#pragma once

#include <kernel/common_server/proto/common.pb.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/geometry/coord.h>
#include <kernel/common_server/library/geometry/polyline.h>
#include <kernel/common_server/library/scheme/scheme.h>

class TGeoAreaId {
private:
    CSA_DEFAULT(TGeoAreaId, TString, Type);
    CSA_DEFAULT(TGeoAreaId, TString, Name);
public:
    TGeoAreaId(const TString& type, const TString& name) : Type(type), Name(name) {
    }

    explicit TGeoAreaId(const NCommonServerProto::TGeoAreaId& proto) {
        if (!DeserializeFromProto(proto)) {
            TFLEventLog::Error("GeoAreaId deserialization failure");
        }
    }

    TString ToString() const {
        return BuildId();
    }

    TString BuildId() const {
        return BuildId(Type, Name);
    }

    NCommonServerProto::TGeoAreaId SerializeToProto() const {
        NCommonServerProto::TGeoAreaId proto;
        proto.SetType(Type);
        proto.SetName(Name);
        return proto;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromProto(const NCommonServerProto::TGeoAreaId& proto) {
        Type = proto.GetType();
        Name = proto.GetName();
        return true;
    }

    static inline TString BuildId(const TString& type, const TString& name) {
        return type + "/" + name;
    }

    static NCS::NScheme::TScheme GetScheme();
};

class TGeoArea {
    CSA_DEFAULT(TGeoArea, TString, Name);
    CSA_DEFAULT(TGeoArea, TString, Type);
    CSA_DEFAULT(TGeoArea, TGeoPolyLine, Polygon);
    CSA_DEFAULT(TGeoArea, TVector<TGeoPolyLine>, Holes);
public:

    TGeoAreaId GetId() const {
        return TGeoAreaId(Type, Name);
    }

    TString BuildId() const {
        return TGeoAreaId::BuildId(Type, Name);
    }

    bool Contains(const TGeoCoord& coord) const {
        if (!Polygon.IsPointInternal(coord))  {
            return false;
        }
        for (auto&& hole : Holes) {
            if (hole.IsPointInternal(coord)) {
                return false;
            }
        }
        return true;
    }

    NJson::TJsonValue GetJsonReport() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        result.InsertValue("object_id", BuildId());
        if (!Type.empty()) {
            result.InsertValue("geoarea_type", Type);
        }
        result.InsertValue("name", Name);
        return result;
    }
};
