#pragma once

#include <util/generic/string.h>

// legacy: pg@ says it's both slower and with worse distribution than normal hash(). Only use for backwards compatibility
struct sgi_hash {
    inline size_t operator()(const char* s) const {
        unsigned long h = 0;
        for (; *s; ++s)
            h = 5 * h + *s;
        return size_t(h);
    }
    inline size_t operator()(const TStringBuf& x) const {
        unsigned long h = 0;
        for (const char *s = x.data(), *e = s + x.size(); s != e; ++s)
            h = 5 * h + *s;
        return size_t(h);
    }
};

struct sgi_hash32: public sgi_hash {
    inline size_t operator()(const char* key) const {
        return (ui32)sgi_hash::operator()(key);
    }
};
