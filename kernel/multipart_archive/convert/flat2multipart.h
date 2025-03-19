#pragma once

#include <util/folder/path.h>
#include <kernel/multipart_archive/abstract/part.h>
#include <kernel/multipart_archive/config/config.h>

namespace NRTYArchive {
    void ConvertFlatArchiveToMultipart(const TFsPath& fatFile, const TFsPath& dataFile, const TFsPath& resultMultipart, const TMultipartConfig&, bool inParallel = false);
    void ConvertMultipartArchiveToFlat(const TFsPath& multipartArchive, const TMultipartConfig& config, const TFsPath& resultFlat);
}
