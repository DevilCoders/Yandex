from util.system.types cimport ui8, ui64
from util.generic.string cimport TString
from util.generic.vector cimport TVector


cdef extern from "library/cpp/geo/geo.h" namespace "NGeo":
    cdef cppclass TPointLL:
        double Lon()
        double Lat()

cdef extern from "library/cpp/geohash/tile/tile.h" namespace "NGeoHash::NTile":
    TVector[TString] CoverCircleFast(double, double, double, ui8) except +

cdef extern from "library/cpp/geohash/geohash.h" namespace "NGeoHash":
    cdef cppclass TBoundingBoxLL:
        double GetMinX()
        double GetMaxX()
        double GetMinY()
        double GetMaxY()

    ui64 Encode(double, double, ui8) except +
    TString EncodeToString(double, double, ui8) except +
    TPointLL DecodeToPoint(const TString&) except +
    TPointLL DecodeToPoint(ui64 hash, ui8 precision) except +
    TBoundingBoxLL DecodeToBoundingBox(const TString&) except +

def encode(double latitude, double longitude, ui8 precision=8):
    return EncodeToString(latitude, longitude, precision)

def encode_to_bits(double latitude, double longitude, ui8 precision=8):
    return Encode(latitude, longitude, precision)

def decode(TString hash):
    point = DecodeToPoint(hash)
    return point.Lat(), point.Lon()

def decode_to_bbox(TString hash):
    bbox = DecodeToBoundingBox(hash)
    return (bbox.GetMinY(), bbox.GetMinX()), (bbox.GetMaxY(), bbox.GetMaxX())

def decode_bits(ui64 bits, ui8 precision=8):
    point = DecodeToPoint(bits, precision)
    return point.Lat(), point.Lon()

def cover_circle(lat, lon, radius, precicion=8):
    return CoverCircleFast(lat, lon, radius, precicion)
