#pragma once

#include <kernel/text_machine/parts/common/features_helper.h>
#include <kernel/text_machine/interface/text_machine.h>

#include <kernel/text_machine/module/module_def.inc>

#include <util/string/builder.h>

namespace NTextMachine {
namespace NCore {
    class TFeaturesConfig {
    public:
        TFeaturesConfig(const TFeaturesHelper& helper)
            : Helper(&helper)
        {}

        const TFeaturesConfigStream& GetStream() const {
            return FeaturesConfig;
        }

        size_t GetNumFeatures() const {
            return NumFeatures;
        }
        size_t GetNumFeatures(size_t slotId) const {
            return SlotsInfo[slotId].Coords.size();
        }
        bool HasFeatures(size_t slotId) const {
            return !SlotsInfo[slotId].Coords.empty();
        }
        TConstCoordsBuffer GetCoords(size_t slotId) const {
            return TConstCoordsBuffer::FromVector(SlotsInfo[slotId].Coords);
        }

        template <typename IterType>
        void ConfigureFromList(IterType begin, IterType end) {
            TDeque<const TFeatureCoords*> coords;
            TVector<size_t> countsBySlot(Helper->GetNumSlots(), 0);

            for (auto iter = begin; iter != end; ++iter) {
                auto idPtr = *iter;
                Y_ASSERT(idPtr);
                if (auto coordsPtr = Helper->GetFeatureCoords(*idPtr)) {
                    coords.push_back(coordsPtr);
                    countsBySlot[coordsPtr->SlotIndex] += 1;
                }
            }

            ConfigureImpl(std::move(coords), std::move(countsBySlot));
       }

       template <typename PredType>
       void ConfigureFromPredicate(const PredType& pred, bool intersect = false) {
           TDeque<const TFeatureCoords*> coords;
           TVector<size_t> countsBySlot(Helper->GetNumSlots(), 0);

           if (intersect) {
               if (Y_UNLIKELY(NeedReorder)) {
                   Y_ASSERT(false);
                   return;
               }

               for (size_t slotId : xrange(Helper->GetNumSlots())) {
                   const auto& ids = Helper->GetFeatureIds(slotId);
                   for (const auto& coordsPtr : SlotsInfo[slotId].Coords) {
                       if (pred(ids[coordsPtr->FeatureIndex])) {
                           coords.push_back(coordsPtr);
                           countsBySlot[slotId] += 1;
                       }
                   }
               }
           } else {
               for (size_t slotId : xrange(Helper->GetNumSlots())) {
                   for (const auto& id : Helper->GetFeatureIds(slotId)) {
                       if (pred(id)) {
                           coords.push_back(Helper->GetFeatureCoords(id));
                           countsBySlot[slotId] += 1;
                       }
                   }
               }
           }

           ConfigureImpl(std::move(coords), std::move(countsBySlot));
       }

       // Must be called before Reorder
       void JoinFeatureIds(const TFeaturesBuffer& features) {
           Y_ASSERT(features.Count() == NumFeatures);

           size_t index = 0;
           for (size_t slotId : xrange(Helper->GetNumSlots())) {
               const auto& ids = Helper->GetFeatureIds(slotId);
               for (auto coordsPtr : SlotsInfo[slotId].Coords) {
                   features[index].SetIdWithHash(&ids[coordsPtr->FeatureIndex]);
                   index += 1;
               }
           }
       }

       template <typename BufType>
       void Reorder(const BufType& features) {
           Y_ASSERT(features.size() == NumFeatures);

           if (!NeedReorder || features.size() == 0) {
               return;
           }

           TDynBitMap visited;
           visited.Reserve(features.size());

           for (size_t i = 0; i < features.size(); ++i) {
               for (; i < features.size() && (Order[i] == i || visited.Test(i)); ++i) {
                   visited.Set(i);
               }

               if (i >= features.size()) {
                   break;
               }

               Y_ASSERT(!visited.Test(i));
               Y_ASSERT(Order[i] < features.size());

               TOptFeature saved = features[i];

               ui32 pos = i;
               ui32 nextPos = Order[i];
               do {
                   Y_ASSERT(nextPos < features.size());
                   features[pos] = features[nextPos];
                   visited.Set(pos);
                   pos = nextPos;
                   nextPos = Order[pos];
               } while (!visited.Test(nextPos));

               Y_ASSERT(nextPos == i);
               features[pos] = saved;
               visited.Set(pos);
           }
       }

    private:
       void ConfigureImpl(TDeque<const TFeatureCoords*>&& coords,
           TVector<size_t>&& countsBySlot)
       {
            NumFeatures = coords.size();
            NeedReorder = false;
            Order.clear();
            Order.reserve(coords.size());

            size_t curOffset = 0;
            SlotsInfo.resize(Helper->GetNumSlots());
            for (size_t slotId : xrange(Helper->GetNumSlots())) {
                SlotsInfo[slotId].Offset = curOffset;
                SlotsInfo[slotId].Coords.clear();
                SlotsInfo[slotId].Coords.reserve(countsBySlot[slotId]);

                curOffset += countsBySlot[slotId];
            }

            for (auto coordsPtr : coords) {
                MACHINE_LOG("Core::TFeaturesConfig", "ConfigureFeature", TStringBuilder{}
                    << "{Id: " << Helper->GetFeatureIds(coordsPtr->SlotIndex)[coordsPtr->FeatureIndex].FullName()
                    << ", Coords: (" << coordsPtr->SlotIndex << ",  " << coordsPtr->FeatureIndex << ")"
                    << ", Order: " << Order.size() << "}");

                auto& slotInfo = SlotsInfo[coordsPtr->SlotIndex];
                const size_t localOffset = slotInfo.Coords.size();
                slotInfo.Coords.push_back(coordsPtr);
                Order.push_back(slotInfo.Offset + localOffset);

                NeedReorder = NeedReorder || Order.back() + 1 != Order.size();
            }

            FeaturesConfig.Clear();
            auto writer = FeaturesConfig.CreateWriter();

            Helper->GetStream().CopyTransform(writer,
                [this](TFeaturesConfigStream::TWriter& theWriter,
                    const TPreparedSlotFeaturesEntry& /*entry*/,
                    size_t slotId)
                {
                    auto& dest = theWriter.Next();
                    dest.Coords = TConstCoordsBuffer::FromVector(SlotsInfo[slotId].Coords);
                });
       }

    private:
        struct TSlotInfo {
            size_t Offset = 0;
            TVector<const TFeatureCoords*> Coords;
        };

        const TFeaturesHelper* Helper = nullptr;

        size_t NumFeatures = 0;
        bool NeedReorder = false;
        TVector<TSlotInfo> SlotsInfo;
        TVector<ui32> Order;
        TFeaturesConfigStream FeaturesConfig;
    };
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>

