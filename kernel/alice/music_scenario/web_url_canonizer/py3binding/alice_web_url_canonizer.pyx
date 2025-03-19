# cython: language_level=3

from _codecs import utf_8_encode, utf_8_decode

from util.generic.string cimport TString

cdef extern from "kernel/alice/music_scenario/web_url_canonizer/lib/web_url_canonizer.h" namespace "NAlice::NMusic":
    cdef TString CanonizeMusicUrl(TString)

def canonize_music_url(url):
    cdef TString mapped_url
    if isinstance(url, str):
        url = utf_8_encode(url)[0]

    return utf_8_decode(CanonizeMusicUrl(url))[0]
