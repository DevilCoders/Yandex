#include "geobase.h"

TLocalInstant IGeobaseProvider::GetLocalInstant(const TInstant instant, const TMaybe<i32> id) const {
    return TLocalInstant(instant, GetTimeZone(id));
}