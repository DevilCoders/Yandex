#include "sequence_contents.h"

#include "serializer.h"

namespace NReqBundle {
namespace NDetail {
    void PrepareBlock(TSequenceElemData& data, TReqBundleDeserializer& deser)
    {
        data.Block = new TBlock;
        deser.ParseBinary(*data.Binary, *data.Block);
    }

    void PrepareBinary(TSequenceElemData& data, TReqBundleSerializer& ser)
    {
        data.Binary = new TBinaryBlock;
        ser.MakeBinary(*data.Block, *data.Binary);
    }
} // NDetail
} // NReqBundle
