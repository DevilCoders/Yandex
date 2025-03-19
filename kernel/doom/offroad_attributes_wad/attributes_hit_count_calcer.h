#pragma once

#include "offroad_attributes_wad_io.h"


namespace NDoom {


struct TAttributesHitCountCalcer {
    static size_t HitCount(const TAttributesHitRange& range) {
        const NOffroad::TTupleSubOffset& start = range.Start();
        const NOffroad::TTupleSubOffset& end = range.End();
        if (start.SubIndex() == end.SubIndex()) {
            Y_ASSERT(start.Offset().Offset() == end.Offset().Offset());
            Y_ASSERT(start.Offset().Index() <= end.Offset().Index());
            return end.Offset().Index() - start.Offset().Index();
        } else {
            Y_ASSERT(start.SubIndex() < end.SubIndex());
            return (end.SubIndex() - start.SubIndex() - 1) * 64 + (64 - start.Offset().Index()) + end.Offset().Index();
        }
    }
};


} // namespace NDoom
