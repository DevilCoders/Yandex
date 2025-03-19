#include "dt_segment.h"

namespace NDT {

namespace {
struct TNextCallbackWrap: public ISegmentCallback {
    const TDTSegmenter& Segmenter;
    ISegmentCallback& Cb;
    TNextCallbackWrap(const TDTSegmenter& seg, ISegmentCallback& cb)
        : Segmenter(seg)
        , Cb(cb)
    {
    }
    void OnSegment(const NIndexerCore::TDirectTextEntry2* entries, size_t from, size_t to, const TLangMask& langs) override {
        Segmenter.Segment(Cb, entries, from, to, langs);
    }
};

} // unnamed namespace

void TDTSegmenter::Segment(ISegmentCallback& cb, const NIndexerCore::TDirectTextEntry2* entries,
    size_t from, size_t to, const TLangMask& langs) const {

    if (!Next) {
        DoSegmentation(cb, entries, from, to, langs);
    } else {
        TNextCallbackWrap wrap(*Next, cb);
        DoSegmentation(wrap, entries, from, to, langs);
    }
}

}
