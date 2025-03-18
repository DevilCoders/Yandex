#include "open_street_map.h"

using namespace NReverseGeocoder;
using namespace NOpenStreetMap;

TString NReverseGeocoder::NOpenStreetMap::FindName(const TKvs& kvs) {
    for (const TKv& kv : kvs)
        if (kv.K == "name:en")
            return kv.V;
    for (const TKv& kv : kvs)
        if (kv.K == "name")
            return kv.V;
    return nullptr;
}
