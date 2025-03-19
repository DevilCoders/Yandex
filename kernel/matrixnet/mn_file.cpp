#include "mn_file.h"

#if defined(MATRIXNET_WITHOUT_ARCADIA)
#include "without_arcadia/util/utility.h"
#endif // defined(MATRIXNET_WITHOUT_ARCADIA)

namespace NMatrixnet {

TMnSseFile::TMnSseFile(const char* const filename)
    : File(filename)
{
    File.Map(0, static_cast<size_t>(File.Length()));
    InitStatic(File.Ptr(), File.MappedSize(), filename);
}

void TMnSseFile::Swap(TMnSseFile& obj) {
    DoSwap(File, obj.File);
    TMnSseInfo::Swap(obj);
}

}
