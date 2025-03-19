#pragma once

#include <extsearch/images/robot/index/protos/metadoc_yt.pb.h>
#include <extsearch/images/protos/t2t.pb.h>
#include <util/generic/vector.h>
#include <util/datetime/base.h>

namespace NImages {

    TVector<float> FillFeatures(const NImages::NIndex::TImgDlErfPB& imgDlErf,
                                const ui32 i2tVersion,
                                const NImages::NIndex::TOmniIndexDataPB& omni,
                                const NImages::NIndex::TErfPB& erf,
                                const TInstant& buildTime,
                                const NImages::TText2textPB& t2tPb);

}
