cdef extern from "library/cpp/geolocation/calcer.h" namespace "NGeolocationFeatures":
    float CalcDistance(float latitude1, float longitude1, float latitude2, float longitude2)


def calc_distance(float latitude1, float longitude1, float latitude2, float longitude2):
    return CalcDistance(latitude1, longitude1, latitude2, longitude2)
