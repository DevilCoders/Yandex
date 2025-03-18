#pragma once

namespace {
    template <typename TValue>
    static inline bool FindInHash(const THashMap<const char*, TValue>& h, const char* key, TValue* p) {
        typename THashMap<const char*, TValue>::const_iterator it = h.find(key);
        if (it == h.end())
            return false;
        *p = it->second;
        return true;
    }
}
