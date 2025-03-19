#include "disk_validation.h"

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

NCloud::NProto::TError ValidateBlockSize(ui64 blockSize)
{
    const auto minBlockSize = 4_KB;
    const auto maxBlockSize = 128_KB;
    if (blockSize < minBlockSize || blockSize > maxBlockSize) {
        return MakeError(NCloud::E_ARGUMENT,
            TStringBuilder() << "block size should be >= " << minBlockSize
            << " and <= " << maxBlockSize);
    }

    if ((blockSize & (blockSize - 1)) != 0) {
        return MakeError(E_ARGUMENT, "block size should be a power of 2");
    }

    return {};
}

}   // namespace NCloud::NBlockStore::NStorage
