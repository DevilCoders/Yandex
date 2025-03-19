#include "filemap.h"
#include "yexception.h"
#include "utility.h"

namespace NMatrixnet {

TFileMap::TFileMap(const std::string& filename)
    : Filename_(filename), File_(nullptr), Length_(0) {
#ifdef _MSC_VER
    auto e = fopen_s(&File_, filename.c_str(), "rb");
    if (e != 0) {
        ythrow TFileError() << "can't open file " << filename << " with mode rb" << ", errno=" << e;
    }
#else
    File_ = fopen(filename.c_str(), "rb");
    if (File_ == 0) {
        ythrow TFileError() << "can't open file " << filename << " with mode rb";
    }
#endif
    if (fseek(File_, 0L, SEEK_END) != 0) {
        ythrow TFileError() << "can't seek to file's end in " << filename;
    }
#ifdef _MSC_VER
    if (fgetpos(File_, &Length_) != 0) {
        ythrow TFileError() << "can't get file position in " << filename;
    }
#else
    Length_ = ftell(File_);
#endif
}

TFileMap::~TFileMap() {
    if (File_)
        fclose(File_);
}

int64_t TFileMap::Length() const {
    return Length_;
}

void TFileMap::Map(int64_t offset, size_t size, const char* dbgName) {
    Unmap();
#ifdef _MSC_VER
    if (fsetpos(File_, &offset) != 0) {
#else
    if (fseek(File_, offset, SEEK_SET) != 0) {
#endif
        ythrow TFileError() << "can't seek to position " << offset << " in " << Filename_;
    }

    if (offset > Length_)
        size = 0;
    if (offset + size > Length_)
        size = static_cast<size_t>(Length_ - offset);
    MappedData_.resize(size);
    if (size != 0) {
        size = fread(MappedData_.data(), 1, size, File_);
        MappedData_.resize(size);
    }
}

const void* TFileMap::Ptr() const {
    return MappedData_.data();
}

size_t TFileMap::MappedSize() const {
    return MappedData_.size();
}

void TFileMap::Unmap() {
    MappedData_.clear();
}

void TFileMap::Swap(TFileMap& other) {
    DoSwap(Filename_, other.Filename_);
    DoSwap(File_, other.File_);
    DoSwap(Length_, other.Length_);
    DoSwap(MappedData_, other.MappedData_);
}

} // namespace NMatrixnet
