#pragma once

#include "mn_sse.h"

#if defined(MATRIXNET_WITHOUT_ARCADIA)
#include "without_arcadia/util/filemap.h"
#else
#include <util/system/defaults.h>
#include <util/system/filemap.h>
#include <util/generic/ptr.h>
#endif

namespace NMatrixnet {

/*! Matrixnet model, constructed from memory mapped mile
 */
class TMnSseFile: public TMnSseInfo {
public:
    TMnSseFile(const char* const filename);

    void Swap(TMnSseFile& obj);

private:
    TFileMap File;
};

typedef TAtomicSharedPtr<const TMnSseFile> TMnSseFilePtr;

}
