#pragma once

#include "types.h"
#include "scatter.h"

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    struct TPreparedSlotFeaturesEntry {
        TVector<TFFIdWithHash> Ids;

        TPreparedSlotFeaturesEntry() = default;
        TPreparedSlotFeaturesEntry(const TSlotFeaturesEntry& entry)
            : Ids(entry.Ids.begin(), entry.Ids.end())
        {}
    };

    using TPreparedFeaturesStream = TStructuralStream<TPreparedSlotFeaturesEntry>;

    class TFeaturesHelper {
    private:
        struct TFFIdPtrHash {
            ui64 operator() (const TFFIdWithHash* x) {
                Y_ASSERT(x);
                return x->Hash();
            }
        };
        struct TFFIdPtrEqual {
            ui64 operator() (const TFFIdWithHash* x, const TFFIdWithHash* y) {
                Y_ASSERT(!!x && !!y);
                return *x == *y;
            }
        };

        TPreparedFeaturesStream PreparedFeatures;
        THashMap<const TFFIdWithHash*, TFeatureCoords, TFFIdPtrHash, TFFIdPtrEqual> CoordsById;

        size_t NumFeatures = 0;

    public:
        TFeaturesHelper(const TFeaturesStream& features) {
            for (const auto& entry : features) {
                NumFeatures += entry.Ids.size();
            };

            CoordsById.reserve(NumFeatures);

            auto writer = PreparedFeatures.CreateWriter();
            features.Copy(writer);

            for (size_t slotIndex : xrange(PreparedFeatures.NumItems())) {
                const auto& entry = PreparedFeatures[slotIndex];

                for (size_t featureIndex : xrange(entry.Ids.size())) {
                    const auto& id = entry.Ids[featureIndex];
                    MACHINE_LOG("Common::TFeaturesHelper", "CollectFeature", TStringBuilder{}
                        << "{Id: " << id.FullName()
                        << ", Coords: (" << slotIndex << ", " << featureIndex << ")}");

                    Y_ASSERT(!CoordsById.contains(&id));
                    CoordsById[&id] = TFeatureCoords{slotIndex, featureIndex};
                }
            }
        }

        const TPreparedFeaturesStream& GetStream() const {
            return PreparedFeatures;
        }

        size_t GetNumFeatures() const {
            return NumFeatures;
        }

        size_t GetNumSlots() const {
            return PreparedFeatures.NumItems();
        }

        size_t GetNumFeatures(size_t slotId) const {
            return GetFeatureIds(slotId).size();
        }

        const TVector<TFFIdWithHash>& GetFeatureIds(size_t slotId) const {
            return PreparedFeatures[slotId].Ids;
        }

        const TFeatureCoords* GetFeatureCoords(const TFFIdWithHash& id) const {
            return CoordsById.FindPtr(&id);
        }
    };
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>

