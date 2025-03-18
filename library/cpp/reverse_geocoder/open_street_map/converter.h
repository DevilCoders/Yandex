#pragma once

#include "parser.h"

#include <library/cpp/reverse_geocoder/proto/region.pb.h>
#include <library/cpp/reverse_geocoder/proto_library/writer.h>

#include <util/generic/hash_set.h>

namespace NReverseGeocoder {
    namespace NOpenStreetMap {
        typedef THashSet<TGeoId> TGeoIdsSet;
        typedef THashMap<TGeoId, TVector<TGeoId>> TWaysMap;
        typedef THashMap<TGeoId, TLocation> TNodesMap;

        class TFindBoundaryWays: public TParser {
        public:
            void ProcessWay(TGeoId, const TKvs&, const TGeoIds&) override;

            void ProcessRelation(TGeoId, const TKvs&, const TReferences&) override;

            const TGeoIdsSet& Ways() const {
                return Ways_;
            }

            void Clear() {
                Ways_.clear();
            }

        private:
            TGeoIdsSet Ways_;
        };

        class TFindBoundaryNodeIds: public TParser {
        public:
            TFindBoundaryNodeIds(const TGeoIdsSet& ways)
                : NeedWays_(&ways)
            {
            }

            void ProcessWay(TGeoId geoId, const TKvs&, const TGeoIds& nodeIds) override;

            void Clear() {
                Ways_.clear();
                Nodes_.clear();
            }

            const TGeoIdsSet& Nodes() const {
                return Nodes_;
            }

            const TWaysMap& Ways() const {
                return Ways_;
            }

        private:
            TWaysMap Ways_;
            const TGeoIdsSet* NeedWays_;
            TGeoIdsSet Nodes_;
        };

        class TFindBoundaryNodes: public TParser {
        public:
            TFindBoundaryNodes(const TGeoIdsSet& nodes)
                : NeedNodes_(&nodes)
            {
            }

            void ProcessNode(TGeoId geoId, const TLocation& location, const TKvs&) override {
                if (NeedNodes_->find(geoId) != NeedNodes_->end())
                    Nodes_[geoId] = location;
            }

            void Clear() {
                Nodes_.clear();
            }

            const TNodesMap& Nodes() const {
                return Nodes_;
            }

        private:
            const TGeoIdsSet* NeedNodes_;
            TNodesMap Nodes_;
        };

        class TConverter: public TParser {
        public:
            TConverter(const TNodesMap& nodes, const TWaysMap& ways, ::NReverseGeocoder::NProto::TWriter* writer)
                : Writer_(writer)
                , Ways_(&ways)
                , Nodes_(&nodes)
                , RegionsNumber_(0)
            {
            }

            void ProcessWay(TGeoId geoId, const TKvs& kvs, const TGeoIds& nodes) override;

            void ProcessRelation(TGeoId geoId, const TKvs& kvs, const TReferences& refs) override;

            size_t RegionsNumber() const {
                return RegionsNumber_;
            }

        private:
            ::NReverseGeocoder::NProto::TWriter* Writer_;
            const TWaysMap* Ways_;
            const TNodesMap* Nodes_;
            size_t RegionsNumber_;
        };

        // Run all convertion in different threads. Read openStreetMap data from inputPath file
        // and write geoBase protobuf into outputPath.
        void RunPoolConvert(const char* inputPath, const char* outputPath, size_t threadsNumber);

    }
}
