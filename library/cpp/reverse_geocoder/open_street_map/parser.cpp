#include "parser.h"

#include <contrib/libs/zlib/zlib.h>

#include <util/generic/utility.h>

using namespace NReverseGeocoder;
using namespace NOpenStreetMap;

static void Unpack(const char* ptr, size_t number, TVector<char>* buffer) {
    unsigned long size = buffer->size();
    int ret = uncompress((unsigned char*)buffer->data(), &size, (unsigned char*)ptr, number);

    if (ret != Z_OK)
        ythrow yexception() << "unpack error: " << ret;

    buffer->resize(size);
}

static bool ParseBasicBlock(const NProto::TBlob& blob, NProto::TBasicBlock* block) {
    TVector<char> buffer(blob.GetRawSize());

    const auto& z = blob.GetZlibData();
    Unpack(z.data(), z.size(), &buffer);

    if (!block->ParseFromArray(buffer.data(), buffer.size()))
        return false;

    return true;
}

static double NormalizeCoordinate(double x) {
    return x * 1e-9;
}

static TLocation MakeLocation(const NProto::TBasicBlock& block, double lat, double lon) {
    TLocation l;
    l.Lat = NormalizeCoordinate(block.GetLatOff() + block.GetGranularity() * lat);
    l.Lon = NormalizeCoordinate(block.GetLonOff() + block.GetGranularity() * lon);
    return l;
}

template <typename TType>
static void MakeKvs(const TType& obj, const NProto::TBasicBlock& block, TKvs& kvs) {
    kvs.resize(obj.KeysSize());
    for (size_t i = 0; i < obj.KeysSize(); ++i) {
        const size_t k = obj.GetKeys(i);
        const size_t v = obj.GetVals(i);
        kvs[i].K = block.GetStringTable().GetS(k);
        kvs[i].V = block.GetStringTable().GetS(v);
    }
}

static void MakeKvs(size_t* offset, const NProto::TDenseNodes& nodes,
                    const NProto::TBasicBlock& block, TKvs& tags) {
    tags.clear();
    tags.reserve(nodes.KvsSize());

    while (*offset < nodes.KvsSize() && nodes.GetKvs(*offset) != 0) {
        const size_t k = nodes.GetKvs(*offset);
        const size_t v = nodes.GetKvs(*offset + 1);
        tags.emplace_back();
        tags.back().K = block.GetStringTable().GetS(k);
        tags.back().V = block.GetStringTable().GetS(v);
        *offset += 2;
    }

    *offset += 1;
}

static TLocation& operator+=(TLocation& a, const TLocation& b) {
    a.Lat += b.Lat;
    a.Lon += b.Lon;
    return a;
}

void NReverseGeocoder::NOpenStreetMap::TParser::ProcessDenseNodes(const NProto::TDenseNodes& nodes,
                                                                  const NProto::TBasicBlock& block, TKvs& kvs) {
    TGeoId geoId = 0;
    TLocation location(0, 0);
    size_t tagsOffset = 0;
    for (size_t i = 0; i < nodes.IdSize(); ++i) {
        geoId += nodes.GetId(i);
        location += MakeLocation(block, nodes.GetLat(i), nodes.GetLon(i));
        MakeKvs(&tagsOffset, nodes, block, kvs);
        ProcessNode(geoId, location, kvs);
        ++DenseNodesProcessed_;
    }
}

void NReverseGeocoder::NOpenStreetMap::TParser::ProcessBasicGroups(const NProto::TBasicBlock& block) {
    TKvs kvs;
    TGeoIds references;
    TReferences relationReferences;

    for (size_t k = 0; k < block.BasicGroupsSize(); ++k) {
        const NProto::TBasicGroup& group = block.GetBasicGroups(k);

        if (!(ProcessingDisabledMask_ & NODE_PROC_DISABLED)) {
            for (size_t i = 0; i < group.NodesSize(); ++i) {
                const NProto::TNode& node = group.GetNodes(i);
                const TLocation location = MakeLocation(block, node.GetLat(), node.GetLon());
                MakeKvs(node, block, kvs);
                ProcessNode(node.GetId(), location, kvs);
                ++NodesProcessed_;
            }

            if (group.HasDenseNodes()) {
                const NProto::TDenseNodes& nodes = group.GetDenseNodes();
                ProcessDenseNodes(nodes, block, kvs);
            }
        }

        if (!(ProcessingDisabledMask_ & WAY_PROC_DISABLED)) {
            for (size_t i = 0; i < group.WaysSize(); ++i) {
                const NProto::TWay& w = group.GetWays(i);

                references.resize(w.RefsSize());

                for (size_t j = 0; j < w.RefsSize(); ++j)
                    references[j] = w.GetRefs(j);
                for (size_t j = 0; j + 1 < w.RefsSize(); ++j)
                    references[j + 1] += references[j];

                MakeKvs(w, block, kvs);
                ProcessWay(w.GetId(), kvs, references);

                ++WaysProcessed_;
            }
        }

        if (!(ProcessingDisabledMask_ & RELATION_PROC_DISABLED)) {
            for (size_t i = 0; i < group.RelationsSize(); ++i) {
                const NProto::TRelation& r = group.GetRelations(i);

                relationReferences.resize(r.MemberIdsSize());

                for (size_t j = 0; j < r.MemberIdsSize(); ++j)
                    relationReferences[j].GeoId = r.GetMemberIds(j);
                for (size_t j = 0; j < r.MemberIdsSize(); ++j)
                    relationReferences[j].Type = (TReference::EType)r.GetMemberTypes(j);
                for (size_t j = 0; j < r.MemberIdsSize(); ++j)
                    relationReferences[j].Role = block.GetStringTable().GetS(r.GetRolesSid(j)).c_str();
                for (size_t j = 0; j + 1 < r.MemberIdsSize(); ++j)
                    relationReferences[j + 1].GeoId += relationReferences[j].GeoId;

                MakeKvs(r, block, kvs);
                ProcessRelation(r.GetId(), kvs, relationReferences);

                ++RelationsProcessed_;
            }
        }
    }
}

void NReverseGeocoder::NOpenStreetMap::TParser::Parse(TReader* reader) {
    NProto::TBlobHeader header;
    NProto::TBlob blob;
    NProto::TBasicBlock block;

    while (true) {
        if (!reader->Read(&header, &blob))
            break;

        if (header.GetType() == "OSMHeader")
            continue;

        if (header.GetType() != "OSMData")
            continue;

        if (blob.HasRaw())
            ythrow yexception() << "Unable parse raw data";

        if (blob.HasLzmaData())
            ythrow yexception() << "Unable parse lzma data";

        if (!ParseBasicBlock(blob, &block))
            ythrow yexception() << "Unable parse block";

        ProcessBasicGroups(block);

        ++BlocksProcessed_;
    }
}

size_t NReverseGeocoder::NOpenStreetMap::OptimalThreadsNumber() {
    ui32 threadsNumber = std::thread::hardware_concurrency();
    if (threadsNumber == 0)
        ++threadsNumber;
    return Min(threadsNumber, static_cast<ui32>(16));
}
