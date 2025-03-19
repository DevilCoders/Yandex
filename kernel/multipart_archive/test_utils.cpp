#include "test_utils.h"
#include "archive_test_base.h"

#include <kernel/multipart_archive/archive_impl/part_thread_safe.h>

#include <library/cpp/logger/global/global.h>
#include <util/system/fs.h>


void InitLog(int level /*= 7*/) {
    if (!GlobalLogInitialized())
    DoInitGlobalLog("console", level, false, false);
}

void NRTYArchive::Clear(const TFsPath& archive) {
    TVector<TFsPath> children;
    archive.Parent().List(children);
    for (auto& f : children) {
        if (f.GetName().StartsWith(archive.GetName()) || f.GetName().StartsWith("~" + archive.GetName())) {
            NFs::Remove(f.GetPath());
        }
    }
}

ui64 NRTYArchive::PutDocumentWithCheck(NRTYArchive::TArchivePartThreadSafe& part, const TBlob& blob, ui32 docid) {
    auto offset = part.TryPutDocument(blob, docid);
    static_assert(sizeof(offset) == sizeof(ui64));
    UNIT_ASSERT_VALUES_UNEQUAL(offset, TPosition::InvalidOffset);
    return offset;
}

const TFsPath NRTYArchive::TArchiveConstructor::DefaultPath = "test";
