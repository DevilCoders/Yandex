#pragma once

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#error This header file is intended for use in the MATRIXNET_WITHOUT_ARCADIA mode only.
#endif

#include "vector.h"

#include <cstdio>
#include <string>
#include <cstdint>

namespace NMatrixnet {

class TFileMap {
public:
    TFileMap(const std::string& filename);
    ~TFileMap();

    int64_t Length() const;

    void Map(int64_t offset, size_t size, const char* dbgName = nullptr);
    const void* Ptr() const;
    size_t MappedSize() const;
    void Unmap();

    void Swap(TFileMap& other);

private:
    std::string Filename_;
    FILE* File_;
    int64_t Length_;
    TVector<char> MappedData_;
};

} // namespace NMatrixnet
