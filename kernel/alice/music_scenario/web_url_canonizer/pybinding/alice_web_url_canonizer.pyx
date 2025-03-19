from util.generic.string cimport TString

cdef extern from "kernel/alice/music_scenario/web_url_canonizer/lib/web_url_canonizer.h" namespace "NAlice::NMusic":
    TString CanonizeMusicUrl(TString)

def canonize_music_url(TString url):
    return CanonizeMusicUrl(url)
