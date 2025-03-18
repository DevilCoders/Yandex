#include "converter.h"

#include <library/cpp/reverse_geocoder/core/polygon.h>
#include <library/cpp/reverse_geocoder/core/region.h>
#include <library/cpp/reverse_geocoder/proto/region.pb.h>
#include <library/cpp/reverse_geocoder/proto_library/writer.h>

#include <util/generic/vector.h>
#include <util/generic/hash_set.h>

using namespace NReverseGeocoder;
using namespace NOpenStreetMap;

using TWayList = TVector<TGeoId>;
using TGraph = THashMap<TGeoId, TVector<TGeoId>>;
using TUsed = THashSet<TGeoId>;

static bool Eq(const char* a, const TString& b) {
    return b == a;
}

// TODO: Make matching with bor here.
#define OP(ev)        \
    if (Eq(ev, kv.V)) \
        return true;

static bool CheckRegionOptions(const TKvs& kvs) {
    for (const TKv& kv : kvs) {
        if (Eq("boundary", kv.K)) {
            OP("administrative");
            OP("maritime");
            OP("nationalPark");
            OP("political");
            OP("protectedArea");
        }
        if (Eq("place", kv.K)) {
            OP("allotments");
            OP("borough");
            OP("city");
            OP("cityBlock");
            OP("continent");
            OP("county");
            OP("district");
            OP("farm");
            OP("hamlet");
            OP("island");
            OP("locality");
            OP("municipality");
            OP("neighbourhood");
            OP("plot");
            OP("province");
            OP("quarter");
            OP("region");
            OP("state");
            OP("suburb");
            OP("town");
            OP("village");
        }
        if (Eq("type", kv.K)) {
            OP("multipolygon");
        }
        if (Eq("building", kv.K)) {
            return true;
        }
        if (Eq("landuse", kv.K)) {
            return true;
        }
        if (Eq("area", kv.K)) {
            OP("yes");
        }
    }
    return false;
}

#undef OP

static TPolygon::EType GetPolygonType(const TString& role) {
    if (!role)
        return TPolygon::TYPE_OUTER;
    if (Eq("outer", role))
        return TPolygon::TYPE_OUTER;
    if (Eq("inner", role))
        return TPolygon::TYPE_INNER;
    return TPolygon::TYPE_UNKNOWN;
}

static bool IsBoundary(const TKvs& kvs) {
    if (!CheckRegionOptions(kvs))
        return false;
    for (const TKv& kv : kvs)
        if (kv.K.StartsWith("name"))
            return true;
    return false;
}

static bool IsBoundaryWay(const TKvs& kvs, const TGeoIds& nodes) {
    return IsBoundary(kvs) && nodes.front() == nodes.back();
}

static bool CheckRole(const TString& role) {
    const TPolygon::EType type = GetPolygonType(role);
    return type == TPolygon::TYPE_INNER || type == TPolygon::TYPE_OUTER;
}

static bool IsWayReference(const TReference& r) {
    return r.Type == TReference::TYPE_WAY && CheckRole(r.Role);
}

void NReverseGeocoder::NOpenStreetMap::TFindBoundaryWays::ProcessWay(TGeoId geoId, const TKvs& kvs, const TGeoIds& refs) {
    if (IsBoundaryWay(kvs, refs))
        Ways_.insert(geoId);
}

void NReverseGeocoder::NOpenStreetMap::TFindBoundaryWays::ProcessRelation(TGeoId, const TKvs& kvs, const TReferences& refs) {
    if (IsBoundary(kvs))
        for (const TReference& ref : refs)
            if (IsWayReference(ref))
                Ways_.insert(ref.GeoId);
}

void NReverseGeocoder::NOpenStreetMap::TFindBoundaryNodeIds::ProcessWay(TGeoId geoId, const TKvs&,
                                                                        const TGeoIds& nodeIds) {
    if (NeedWays_->find(geoId) != NeedWays_->end()) {
        for (const TGeoId& nodeId : nodeIds) {
            Nodes_.insert(nodeId);
            Ways_[geoId].push_back(nodeId);
        }
    }
}

static void LocationAppend(const TNodesMap& nodes, ::NReverseGeocoder::NProto::TPolygon* poly,
                           TGeoId nodeId) {
    const TLocation& node = nodes.at(nodeId);
    ::NReverseGeocoder::NProto::TLocation* location = poly->AddLocations();
    location->SetLat(node.Lat);
    location->SetLon(node.Lon);
}

static void KvsAppend(const TKv& kv, ::NReverseGeocoder::NProto::TRegion* region) {
    ::NReverseGeocoder::NProto::TKv* x = region->AddKvs();
    x->SetK(kv.K);
    x->SetV(kv.V);
}

static TGeoId GeneratePolygonId() {
    // TODO: Make good polygonId generation.
    return 0;
}

void NReverseGeocoder::NOpenStreetMap::TConverter::ProcessWay(TGeoId geoId, const TKvs& kvs, const TGeoIds& refs) {
    if (!IsBoundaryWay(kvs, refs))
        return;

    ::NReverseGeocoder::NProto::TRegion region;
    region.SetRegionId(geoId);

    ::NReverseGeocoder::NProto::TPolygon* polygon = region.AddPolygons();
    polygon->SetPolygonId(GeneratePolygonId());
    polygon->SetType(::NReverseGeocoder::NProto::TPolygon::TYPE_OUTER);

    for (const TGeoId& id : refs)
        LocationAppend(*Nodes_, polygon, id);

    for (const TKv& kv : kvs)
        KvsAppend(kv, &region);

    Writer_->Write(region);
    ++RegionsNumber_;
}

static void SaveLocations(const TGraph& graph, const TNodesMap& nodes, TGeoId geoId1,
                          ::NReverseGeocoder::NProto::TPolygon* polygon, TUsed* used) {
    TVector<TGeoId> stack;
    stack.reserve(2 * graph.size());

    stack.push_back(geoId1);

    while (!stack.empty()) {
        TGeoId geoId2 = stack.back();
        stack.pop_back();

        used->insert(geoId2);
        LocationAppend(nodes, polygon, geoId2);

        if (graph.find(geoId2) != graph.end())
            for (const TGeoId& nodeId : graph.at(geoId2))
                if (used->find(nodeId) == used->end())
                    stack.push_back(nodeId);
    }
}

void NReverseGeocoder::NOpenStreetMap::TConverter::ProcessRelation(TGeoId geoId, const TKvs& kvs, const TReferences& refs) {
    if (!IsBoundary(kvs))
        return;

    bool allWaysFound = true;
    for (const TReference& r : refs)
        if (IsWayReference(r) && Ways_->find(r.GeoId) == Ways_->end())
            allWaysFound = false;

    if (!allWaysFound) {
        LogWarning("Not found all ways for %s (%lu)", FindName(kvs).c_str(), geoId);
        return;
    }

    TGraph graph;

    for (const TReference& r : refs) {
        if (!IsWayReference(r))
            continue;

        const TWayList& w = Ways_->at(r.GeoId);
        for (TNumber i = 0; i + 1 < w.size(); ++i) {
            if (graph[w[i]].size() < 2 && graph[w[i + 1]].size() < 2) {
                graph[w[i]].push_back(w[i + 1]);
                graph[w[i + 1]].push_back(w[i]);
            }
        }
    }

    ::NReverseGeocoder::NProto::TRegion region;
    region.SetRegionId(geoId);

    TUsed used;

    for (const TReference& r : refs) {
        if (!IsWayReference(r))
            continue;

        const TWayList& w = Ways_->at(r.GeoId);
        for (TNumber i = 0; i < w.size(); ++i) {
            if (used.find(w[i]) != used.end())
                continue;

            ::NReverseGeocoder::NProto::TPolygon* polygon = region.AddPolygons();
            polygon->SetPolygonId(GeneratePolygonId());
            polygon->SetType((::NReverseGeocoder::NProto::TPolygon::EType)GetPolygonType(r.Role));

            SaveLocations(graph, *Nodes_, w[i], polygon, &used);
        }
    }

    if (region.PolygonsSize() > 0) {
        for (const TKv& kv : kvs)
            KvsAppend(kv, &region);

        Writer_->Write(region);
        ++RegionsNumber_;
    }
}

void NReverseGeocoder::NOpenStreetMap::RunPoolConvert(const char* inputPath, const char* outputPath, size_t threadsNumber) {
    LogInfo("Run pool convert (%lu threads)", threadsNumber);

    TStopWatch stopWatch;
    stopWatch.Run();

    TGeoIdsSet wayIds;

    {
        LogInfo("Grep boundary ways...");

        TVector<TFindBoundaryWays> greps;
        for (size_t i = 0; i < threadsNumber; ++i)
            greps.emplace_back();

        TPoolParser parser;
        parser.Parse(inputPath, greps);

        for (size_t i = 0; i < greps.size(); ++i) {
            wayIds.insert(greps[i].Ways().begin(), greps[i].Ways().end());
            greps[i].Clear();
        }

        LogInfo("Grep boundary ways done, wayIds = %lu", wayIds.size());
    }

    TGeoIdsSet nodeIds;
    TWaysMap ways;

    {
        LogInfo("Grep boundary node ids...");

        TVector<TFindBoundaryNodeIds> greps;
        for (size_t i = 0; i < threadsNumber; ++i)
            greps.emplace_back(wayIds);

        TPoolParser parser;
        parser.Parse(inputPath, greps);

        for (size_t i = 0; i < greps.size(); ++i) {
            nodeIds.insert(greps[i].Nodes().begin(), greps[i].Nodes().end());
            ways.insert(greps[i].Ways().begin(), greps[i].Ways().end());
            greps[i].Clear();
        }

        LogInfo("Grep boundary node ids done, nodeIds = %lu, ways = %lu",
                nodeIds.size(), ways.size());
    }

    TNodesMap nodes;

    {
        LogInfo("Grep boundary nodes...");

        TVector<TFindBoundaryNodes> greps;
        for (size_t i = 0; i < threadsNumber; ++i)
            greps.emplace_back(nodeIds);

        TPoolParser parser;
        parser.Parse(inputPath, greps);

        for (size_t i = 0; i < greps.size(); ++i) {
            nodes.insert(greps[i].Nodes().begin(), greps[i].Nodes().end());
            greps[i].Clear();
        }

        LogInfo("Grep boundary nodes done, nodes = %lu", nodes.size());
    }

    {
        LogInfo("Convert...");

        ::NReverseGeocoder::NProto::TWriter protoWriter(outputPath);

        TVector<TConverter> converters;
        for (size_t i = 0; i < threadsNumber; ++i)
            converters.emplace_back(nodes, ways, &protoWriter);

        TPoolParser parser;
        parser.Parse(inputPath, converters);

        size_t regionsNumber = 0;
        for (size_t i = 0; i < threadsNumber; ++i)
            regionsNumber += converters[i].RegionsNumber();

        LogInfo("Convert done, number of regions = %lu", regionsNumber);
    }

    float const seconds = stopWatch.Get();
    LogInfo("Run pool parse done in %.3f seconds (%.3f minutes)", seconds, seconds / 60.0);
}
