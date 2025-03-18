#pragma once

#include <library/cpp/reverse_geocoder/open_street_map/parser.h>

namespace NReverseGeocoder {
    namespace NOpenStreetMap {
        class TSimpleCounter: public TParser {
        public:
            TSimpleCounter()
                : NodesNumber_(0)
                , WaysNumber_(0)
                , RelationsNumber_(0)
            {
            }

            TSimpleCounter(TSimpleCounter&& c)
                : TParser(std::forward<TParser>(c))
                , NodesNumber_(0)
                , WaysNumber_(0)
                , RelationsNumber_(0)
            {
                DoSwap(NodesNumber_, c.NodesNumber_);
                DoSwap(WaysNumber_, c.WaysNumber_);
                DoSwap(RelationsNumber_, c.RelationsNumber_);
            }

            void ProcessNode(TGeoId, const TLocation&, const TKvs&) override {
                ++NodesNumber_;
            }

            void ProcessWay(TGeoId, const TKvs&, const TGeoIds&) override {
                ++WaysNumber_;
            }

            void ProcessRelation(TGeoId, const TKvs&, const TReferences&) override {
                ++RelationsNumber_;
            }

            size_t NodesNumber() const {
                return NodesNumber_;
            }

            size_t WaysNumber() const {
                return WaysNumber_;
            }

            size_t RelationsNumber() const {
                return RelationsNumber_;
            }

        private:
            size_t NodesNumber_;
            size_t WaysNumber_;
            size_t RelationsNumber_;
        };

    }
}
