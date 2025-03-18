#pragma once

#include <util/system/defaults.h>
#include <util/system/yassert.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

#include "token_classifier_interface.h"

namespace NTokenClassification {
    struct TInterval {
        TInterval();
        TInterval(const wchar16* begin, const wchar16* end);

        size_t Length() const;

        bool Empty() const;
        bool Initialized() const;

        const wchar16* Begin;
        const wchar16* End;
    };

    class TMarkupGroup {
    public:
        typedef TVector<TUtf16String> TWStringLayer;

        TMarkupGroup() {
        }

        TMarkupGroup(const TInterval& groupInterval)
            : GroupInterval(groupInterval)
        {
        }

        inline bool Initialized() const {
            return GroupInterval.Initialized();
        }

        inline size_t MarkupLayersNumber() const {
            return MarkupLayers.size();
        }

        inline bool Empty() const {
            return MarkupLayers.empty();
        }

        template <class TLayerId>
        inline bool HasMarkupLayer(const TLayerId& id) const {
            TLayerDefaultId defaultId = ToDefaultId(id);

            return MarkupLayers.find(defaultId) != MarkupLayers.end();
        }

        inline const TInterval& GetGroupInterval() const {
            return GroupInterval;
        }

        inline void SetGroupInterval(const TInterval& groupInterval) {
            GroupInterval = groupInterval;
            MarkupLayers.clear();
        }

        template <class TLayerId>
        inline size_t MarkupLayerSize(const TLayerId& id) const {
            TLayerDefaultId defaultId = ToDefaultId(id);

            TMarkupLayersConstIterator layer = MarkupLayers.find(defaultId);
            Y_ASSERT(layer != MarkupLayers.end());

            return layer->second.size();
        }

        template <class TLayerId>
        inline const TInterval& GetMarkupInterval(const TLayerId& id, size_t index) const {
            TLayerDefaultId defaultLayerId = ToDefaultId(id);
            TMarkupLayersConstIterator layer = MarkupLayers.find(defaultLayerId);
            Y_ASSERT(layer != MarkupLayers.end());

            return layer->second[index];
        }

        template <class TLayerId>
        inline void AddMarkupInterval(const TLayerId& id, const TInterval& markupInterval) {
            TLayerDefaultId defaultId = ToDefaultId(id);
            TMarkupLayersIterator layer = MarkupLayers.find(defaultId);
            Y_ASSERT(layer != MarkupLayers.end() && Initialized() && markupInterval.Initialized() &&
                     markupInterval.Begin >= GroupInterval.Begin && markupInterval.End <= GroupInterval.End);

            layer->second.push_back(markupInterval);
        }

        template <class TLayerId>
        inline void AddMarkupLayer(const TLayerId& id) {
            TLayerDefaultId defaultId = ToDefaultId(id);
            Y_ASSERT(MarkupLayers.find(defaultId) == MarkupLayers.end());

            MarkupLayers.insert(TMarkupLayersEntry(defaultId, TMarkupLayer()));
        }

        template <class TLayerId>
        inline void RemoveMarkupLayer(const TLayerId& id) {
            TLayerDefaultId defaultId = ToDefaultId(id);
            TMarkupLayersIterator layer = MarkupLayers.find(defaultId);
            Y_ASSERT(layer != MarkupLayers.end());

            MarkupLayers.erase(layer);
        }

        template <class TLayerId>
        inline void GetLayerAsWStringLayer(const TLayerId& id, TWStringLayer& wStringLayer) const {
            TLayerDefaultId defaultLayerId = ToDefaultId(id);
            TMarkupLayersConstIterator layer = MarkupLayers.find(defaultLayerId);
            Y_ASSERT(layer != MarkupLayers.end());

            const TMarkupLayer& markupLayer = layer->second;

            wStringLayer.clear();
            wStringLayer.reserve(markupLayer.size());
            for (auto interval : markupLayer) {
                wStringLayer.emplace_back(interval.Begin, interval.End);
            }
        }

    private:
        typedef i32 TLayerDefaultId;

        typedef TVector<TInterval> TMarkupLayer;
        typedef THashMap<TLayerDefaultId, TMarkupLayer> TMarkupLayers;

        typedef std::pair<TLayerDefaultId, TMarkupLayer> TMarkupLayersEntry;

        typedef TMarkupLayer::iterator TMarkupLayerIterator;
        typedef TMarkupLayer::const_iterator TMarkupLayerConstIterator;

        typedef TMarkupLayers::iterator TMarkupLayersIterator;
        typedef TMarkupLayers::const_iterator TMarkupLayersConstIterator;

        template <class TLayerId>
        static inline TLayerDefaultId ToDefaultId(TLayerId id) {
            return TLayerDefaultId(id);
        }

        TInterval GroupInterval;
        TMarkupLayers MarkupLayers;
    };

    class TMultitokenMarkupGroups {
    public:
        TMultitokenMarkupGroups() {
        }

        inline bool Empty() const {
            return MarkupGroups.empty();
        }

        inline size_t Size() const {
            return MarkupGroups.size();
        }

        template <class TGroupId>
        inline bool HasMarkupGroup(const TGroupId& id) const {
            TGroupDefaultId defaultId = ToDefaultId(id);

            return MarkupGroups.find(defaultId) != MarkupGroups.end();
        }

        template <class TGroupId>
        inline void AddMarkupGroup(const TGroupId& id) {
            TGroupDefaultId defaultId = ToDefaultId(id);
            Y_ASSERT(MarkupGroups.find(defaultId) == MarkupGroups.end());

            MarkupGroups.insert(TMarkupGroupsEntry(defaultId, TMarkupGroup()));
        }

        template <class TGroupId>
        inline void AddMarkupGroup(const TGroupId& id, const TInterval& groupInterval) {
            TGroupDefaultId defaultId = ToDefaultId(id);
            Y_ASSERT(MarkupGroups.find(defaultId) == MarkupGroups.end());

            MarkupGroups.insert(TMarkupGroupsEntry(defaultId, TMarkupGroup(groupInterval)));
        }

        template <class TGroupId>
        inline const TMarkupGroup& GetMarkupGroup(const TGroupId& id) const {
            TGroupDefaultId defaultId = ToDefaultId(id);

            TMarkupGroupsConstIterator group = MarkupGroups.find(defaultId);
            Y_ASSERT(group != MarkupGroups.end());

            return group->second;
        }

        template <class TGroupId>
        inline TMarkupGroup& GetMarkupGroup(const TGroupId& id) {
            TGroupDefaultId defaultId = ToDefaultId(id);

            TMarkupGroupsIterator group = MarkupGroups.find(defaultId);
            Y_ASSERT(group != MarkupGroups.end());

            return group->second;
        }

    private:
        typedef i32 TGroupDefaultId;

        typedef THashMap<TGroupDefaultId, TMarkupGroup> TMarkupGroups;

        typedef std::pair<TGroupDefaultId, TMarkupGroup> TMarkupGroupsEntry;

        typedef TMarkupGroups::iterator TMarkupGroupsIterator;
        typedef TMarkupGroups::const_iterator TMarkupGroupsConstIterator;

        template <class TGroupId>
        static inline TGroupDefaultId ToDefaultId(const TGroupId& id) {
            return TGroupDefaultId(id);
        }

        TMarkupGroups MarkupGroups;
    };

    using TDelimeterMap = TVector<THashSet<wchar16>>;

    static inline size_t GetBottomLayerIndex(wchar16 current,
                                             const TDelimeterMap& delimiters) {
        for (size_t layerIndex = 0; layerIndex < delimiters.size(); ++layerIndex) {
            if (delimiters[layerIndex].find(current) != delimiters[layerIndex].end()) {
                return layerIndex;
            }
        }

        return delimiters.size();
    }

    template <class TLayerId>
    void FillGroupLayers(const TVector<TLayerId>& layers,
                         const TDelimeterMap& delimiters,
                         TMarkupGroup& markupGroup) {
        typedef TVector<const wchar16*> TLayerIntervalBeginChars;

        const wchar16* begin = markupGroup.GetGroupInterval().Begin;
        const wchar16* end = markupGroup.GetGroupInterval().End;

        for (size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
            markupGroup.AddMarkupLayer(layers[layerIndex]);
        }

        TLayerIntervalBeginChars layersIntervalBeginChars(layers.size(), begin);
        const wchar16* current = begin;

        while (current != end) {
            size_t bottomLayerIndex = GetBottomLayerIndex(*current, delimiters);
            for (size_t layerIndex = bottomLayerIndex; layerIndex < layers.size(); ++layerIndex) {
                if (layersIntervalBeginChars[layerIndex] != current) {
                    markupGroup.AddMarkupInterval(layers[layerIndex],
                                                  TInterval(layersIntervalBeginChars[layerIndex], current));
                }

                layersIntervalBeginChars[layerIndex] = current + 1;
            }
            ++current;
        }

        for (size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
            if (layersIntervalBeginChars[layerIndex] != end) {
                markupGroup.AddMarkupInterval(layers[layerIndex],
                                              TInterval(layersIntervalBeginChars[layerIndex], end));
            }
        }
    }

}
