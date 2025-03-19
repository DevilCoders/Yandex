#pragma once
#include <util/generic/map.h>

class TMapProcessor {
public:
    template <class TId, class TObject>
    static TMaybe<TObject> GetValue(const TMap<TId, TObject>& map, const TId& key) {
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        }
        return {};
    }

    template <class TId, class TObject>
    static const TObject* GetValuePtr(const TMap<TId, TObject>& map, const TId& key) {
        auto it = map.find(key);
        if (it != map.end()) {
            return &it->second;
        }
        return nullptr;
    }

    template <class TId, class TObject>
    static TObject* GetValueMutablePtr(TMap<TId, TObject>& map, const TId& key) {
        auto it = map.find(key);
        if (it != map.end()) {
            return &it->second;
        }
        return nullptr;
    }

    template <class TId, class TObject>
    static TObject* GetValueMutablePtr(TMap<TId, TObject*>& map, const TId& key) {
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        }
        return nullptr;
    }

    template <class TId, class TObject>
    static const TObject* GetValuePtr(const TMap<TId, const TObject*>& map, const TId& key) {
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        }
        return nullptr;
    }

    template <class TId, class TObject, class TExceptPolicy>
    static const TObject* GetValuePtr(const TMap<TId, const TObject*>& map, const TMaybe<TId, TExceptPolicy>& key) {
        if (!key) {
            return nullptr;
        }
        return GetValuePtr(map, *key);
    }

    template <class TId, class TObject, class TExceptPolicy>
    static const TObject* GetValuePtr(const TMap<TId, TObject>& map, const TMaybe<TId, TExceptPolicy>& key) {
        if (!key) {
            return nullptr;
        }
        return GetValuePtr(map, *key);
    }

    template <class TId, class TObject>
    static TObject GetValueDef(const TMap<TId, TObject>& map, const TId& key, const TObject& defaultValue) {
        return GetValue(map, key).GetOrElse(defaultValue);
    }

    template <class TId, class TObject, class TExceptPolicy>
    static TObject GetValueDef(const TMap<TId, TObject>& map, const TMaybe<TId, TExceptPolicy>& key, const TObject& defaultValue) {
        return GetValue(map, key).GetOrElse(defaultValue);
    }
};
