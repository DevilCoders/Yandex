#include "archive_manager.h"

namespace NRTYArchive {
    IArchiveManager::IArchiveManager(const IArchivePart::TConstructContext& constructCtx)
        : ConstructCtx(constructCtx)
    {
        CheckConstructContext();
    }

    void IArchiveManager::CheckConstructContext() const {
        VERIFY_WITH_LOG(!ConstructCtx.WithoutSizes || (ConstructCtx.Type == IArchivePart::RAW), "Only RAW archives can be written wihtout blob sizes");
    }
}
