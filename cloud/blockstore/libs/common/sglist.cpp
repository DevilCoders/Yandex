#include "sglist.h"

#include <util/string/builder.h>

namespace NCloud::NBlockStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

bool AppendBufferToSgList(TSgList& sglist, TBlockDataRef buffer, ui32 blockSize)
{
    Y_VERIFY(blockSize != 0);

    if (buffer.Size() == 0 || buffer.Size() % blockSize != 0) {
        return false;
    }

    auto count = buffer.Size() / blockSize;
    auto* data = buffer.Data();

    if (data) {
        for (size_t i = 0; i < count; ++i) {
            sglist.emplace_back(data, blockSize);
            data += blockSize;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            sglist.emplace_back(TBlockDataRef::CreateZeroBlock(blockSize));
        }
    }

    return true;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

size_t SgListGetSize(const TSgList& sglist)
{
    size_t len = 0;
    for (const auto& vec: sglist) {
        len += vec.Size();
    }
    return len;
}

size_t SgListCopy(const TSgList& srcList, const TSgList& dstList)
{
    size_t bytesCount = 0;

    const char* src = nullptr;
    size_t srcLen = 0;

    char* dst = nullptr;
    size_t dstLen = 0;

    size_t srcIndex = 0, dstIndex = 0;
    for (;;) {
        if (!srcLen) {
            if (srcIndex < srcList.size()) {
                const auto& block = srcList[srcIndex++];
                src = block.Data();
                srcLen = block.Size();
            } else {
                break;
            }
        }

        if (!dstLen) {
            if (dstIndex < dstList.size()) {
                const auto& block = dstList[dstIndex++];
                dst = (char*)block.Data();
                dstLen = block.Size();
            } else {
                break;
            }
        }

        size_t toCopy = Min(srcLen, dstLen);
        if (src) {
            memcpy(dst, src, toCopy);
            src += toCopy;
        } else {
            // If block data is not set this means we need to return zeroes here
            memset(dst, 0, toCopy);
        }

        dst += toCopy;
        srcLen -= toCopy;
        dstLen -= toCopy;
        bytesCount += toCopy;
    }

    return bytesCount;
}

size_t SgListCopy(TBlockDataRef srcBuf, const TSgList& dstList)
{
    size_t bytesCount = 0;

    const char* src = srcBuf.Data();
    size_t srcLen = srcBuf.Size();

    char* dst = nullptr;
    size_t dstLen = 0;

    size_t dstIndex = 0;
    while (srcLen) {
        if (!dstLen) {
            if (dstIndex < dstList.size()) {
                const auto& block = dstList[dstIndex++];
                dst = (char*)block.Data();
                dstLen = block.Size();
            } else {
                break;
            }
        }

        size_t toCopy = Min(srcLen, dstLen);
        memcpy(dst, src, toCopy);
        src += toCopy;
        dst += toCopy;
        srcLen -= toCopy;
        dstLen -= toCopy;
        bytesCount += toCopy;
    }

    return bytesCount;
}

size_t SgListCopy(const TSgList& srcList, TBlockDataRef dstBuf)
{
    size_t bytesCount = 0;

    const char* src = nullptr;
    size_t srcLen = 0;

    char* dst = (char*)dstBuf.Data();
    size_t dstLen = dstBuf.Size();

    size_t srcIndex = 0;
    while (dstLen) {
        if (!srcLen) {
            if (srcIndex < srcList.size()) {
                const auto& block = srcList[srcIndex++];
                src = block.Data();
                srcLen = block.Size();
            } else {
                break;
            }
        }

        size_t toCopy = Min(srcLen, dstLen);
        if (src) {
            memcpy(dst, src, toCopy);
            src += toCopy;
        } else {
            // If block data is not set this means we need to return zeroes here
            memset(dst, 0, toCopy);
        }

        dst += toCopy;
        srcLen -= toCopy;
        dstLen -= toCopy;
        bytesCount += toCopy;
    }

    return bytesCount;
}

TResultOrError<TSgList> SgListNormalize(TBlockDataRef buffer, ui32 blockSize)
{
    TSgList result;

    if (!AppendBufferToSgList(result, buffer, blockSize)) {
        return MakeError(E_ARGUMENT, TStringBuilder()
            << "failed to normalize buffer."
            << " BufferSize = " << buffer.Size()
            << " BlockSize = " << blockSize);
    }

    return result;
}

TResultOrError<TSgList> SgListNormalize(TSgList sglist, ui32 blockSize)
{
    bool normalized = true;
    for (const auto& buffer: sglist) {
        if (buffer.Size() != blockSize) {
            normalized = false;
            break;
        }
    }

    if (normalized) {
        return std::move(sglist);
    }

    TSgList result(Reserve(sglist.size()));

    for (const auto& buffer: sglist) {
        if (!AppendBufferToSgList(result, buffer, blockSize)) {
            return MakeError(E_ARGUMENT, TStringBuilder()
                << "failed to normalize buffer."
                << " BufferSize = " << buffer.Size()
                << " BlockSize = " << blockSize);
        }
    }

    return result;
}

}   // namespace NCloud::NBlockStore
