#include "owner.h"

#include <util/system/fs.h>


void NRTYArchive::HardLinkOrCopy(const TFsPath& from, const TFsPath& to) {
    TVector<TFsPath> files;
    from.Parent().List(files);
    for (const auto& file : files)
        if (file.GetName().StartsWith(from.GetName()))
            NFs::HardLinkOrCopy(file, (to.GetPath() + file.GetName().substr(from.GetName().size())).data());
}
