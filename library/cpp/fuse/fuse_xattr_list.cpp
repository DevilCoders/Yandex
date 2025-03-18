#include "fuse_xattr_list.h"

#include <string.h>

namespace NFuse {

    bool TFuseXattrList::Add(TStringBuf name) {
        const size_t entrySize = name.Size() + 1;
        ReadSize_ += entrySize;

        if (ReadSize_ > MaxSize_) {
            return MaxSize_ == 0;
        }

        if (!Buf_.Get()) {
            Buf_.Reset(new char[MaxSize_]);
        }

        memcpy(Buf_.Get() + ReadSize_ - entrySize, name.Data(), name.Size());
        Buf_[ReadSize_ - 1] = '\0';

        return true;
    }

} // namespace NFuse
