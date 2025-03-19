#include "reqbundle_contents.h"

#include "reqbundle_accessors.h"

namespace NReqBundle {
namespace NDetail {
    void ReplaceSequence(TReqBundleData& data, const TSequencePtr& seqPtr)
    {
        Y_ASSERT(data.Sequence && seqPtr);
        data.Sequence = seqPtr;
    }
} // NDetail
} // NReqBundle
