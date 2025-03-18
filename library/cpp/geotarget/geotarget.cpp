#include <cstdio>

#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/singleton.h>

#include <util/network/socket.h>

#include <util/system/compat.h>
#include <util/system/yassert.h>

#include "geotarget.h"

class TGeoTargetData {
public:
    inline int LoadGeoTarget(const char* fn) {
        FILE* F = fopen(fn, "r");

        if (F == nullptr) {
            warn("LoadGeoTarg: %s", fn);
            return 1;
        }

        Clear();

        unsigned int l, h, t;

        while (fscanf(F, "%u %u %u\n", &l, &h, &t) == 3) {
            iplow.push_back(l);
            iphigh.push_back(h);
            geotag.push_back(t);
        }

        fclose(F);

        return 0;
    }

    inline int FindGeoTarget(unsigned int ip) {
        ivector::const_iterator hb = LowerBound(iphigh.begin(), iphigh.end(), ip);

        if (hb == iphigh.end())
            return -1;

        if (iplow[hb - iphigh.begin()] > ip)
            return -1;

        return geotag[hb - iphigh.begin()];
    }

    inline int FindGeoTarget(const char* ip_str) {
        Y_ASSERT(ip_str && *ip_str);

        in_addr ia;

        if (inet_aton(ip_str, &ia) != 0) {
            ui32 ip = ntohl(ia.s_addr);

            return FindGeoTarget(ip);
        }

        return -1;
    }

    inline void Clear() {
        iplow.clear();
        iphigh.clear();
        geotag.clear();
    }

private:
    typedef TVector<unsigned int> ivector;
    ivector iplow;
    ivector iphigh;
    ivector geotag;
};

static inline TGeoTargetData* GeoTarget() {
    return Singleton<TGeoTargetData>();
}

int LoadGeoTarget(const char* fn) {
    return GeoTarget()->LoadGeoTarget(fn);
}

int FindGeoTarget(unsigned int ip) {
    return GeoTarget()->FindGeoTarget(ip);
}

int FindGeoTarget(const char* ipStr) {
    return GeoTarget()->FindGeoTarget(ipStr);
}
