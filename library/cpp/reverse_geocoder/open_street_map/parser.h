#pragma once

#include "open_street_map.h"
#include "reader.h"

#include <library/cpp/reverse_geocoder/core/location.h>
#include <library/cpp/reverse_geocoder/library/log.h>
#include <library/cpp/reverse_geocoder/library/stop_watch.h>
#include <library/cpp/reverse_geocoder/open_street_map/proto/open_street_map.pb.h>

#include <util/stream/file.h>

#include <thread>

namespace NReverseGeocoder {
    namespace NOpenStreetMap {
        class TParser: public TNonCopyable {
        public:
            TParser()
                : BlocksProcessed_(0)
                , NodesProcessed_(0)
                , DenseNodesProcessed_(0)
                , RelationsProcessed_(0)
                , WaysProcessed_(0)
                , ProcessingDisabledMask_(0)
            {
            }

            TParser(TParser&& p)
                : TParser()
            {
                DoSwap(BlocksProcessed_, p.BlocksProcessed_);
                DoSwap(NodesProcessed_, p.NodesProcessed_);
                DoSwap(DenseNodesProcessed_, p.DenseNodesProcessed_);
                DoSwap(RelationsProcessed_, p.RelationsProcessed_);
                DoSwap(WaysProcessed_, WaysProcessed_);
                DoSwap(ProcessingDisabledMask_, ProcessingDisabledMask_);
            }

            virtual ~TParser() = default;

            void Parse(TReader* reader);

            ui64 BlocksProcessed() const {
                return BlocksProcessed_;
            }

            ui64 NodesProcessed() const {
                return NodesProcessed_;
            }

            ui64 DenseNodesProcessed() const {
                return DenseNodesProcessed_;
            }

            ui64 RelationsProcessed() const {
                return RelationsProcessed_;
            }

            ui64 WaysProcessed() const {
                return WaysProcessed_;
            }

        protected:
            virtual void ProcessNode(TGeoId, const TLocation&, const TKvs&) {
                ProcessingDisabledMask_ |= NODE_PROC_DISABLED;
            }

            virtual void ProcessWay(TGeoId, const TKvs&, const TGeoIds&) {
                ProcessingDisabledMask_ |= WAY_PROC_DISABLED;
            }

            virtual void ProcessRelation(TGeoId, const TKvs&, const TReferences&) {
                ProcessingDisabledMask_ |= RELATION_PROC_DISABLED;
            }

        private:
            enum processingDisabledT {
                NODE_PROC_DISABLED = (1u << 0),
                WAY_PROC_DISABLED = (1u << 1),
                RELATION_PROC_DISABLED = (1u << 2),
            };

            void ProcessBasicGroups(const NProto::TBasicBlock& block);

            void ProcessDenseNodes(const NProto::TDenseNodes& nodes,
                                   const NProto::TBasicBlock& block, TKvs& kvsBuffer);

            ui64 BlocksProcessed_;
            ui64 NodesProcessed_;
            ui64 DenseNodesProcessed_;
            ui64 RelationsProcessed_;
            ui64 WaysProcessed_;
            ui64 ProcessingDisabledMask_;
        };

        class TPoolParser: public TNonCopyable {
        public:
            TPoolParser() {
            }

            template <typename TContainer>
            void Parse(TReader* reader, TContainer& container) {
                TVector<std::thread> threads(container.size());
                for (ui64 i = 0; i < threads.size(); ++i) {
                    threads[i] = std::thread([&container, &reader, i]() {
                        container[i].Parse(reader);
                    });
                }
                for (ui64 i = 0; i < threads.size(); ++i)
                    threads[i].join();
            }

            template <typename TContainer>
            void Parse(const char* path, TContainer& container) {
                LogInfo("Run parser on %s", path);

                TStopWatch stopWatch;
                stopWatch.Run();

                {
                    TFileInput inputStream(path);
                    TReader reader(&inputStream);
                    Parse(&reader, container);
                }

                const float seconds = stopWatch.Get();
                LogInfo("Parsed %s in %.3f seconds (%.3f minutes)",
                        path, seconds, seconds / 60.0);
            }
        };

        size_t OptimalThreadsNumber();

    }
}
