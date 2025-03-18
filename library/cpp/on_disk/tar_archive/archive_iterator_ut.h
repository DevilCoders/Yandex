#pragma once
#include <util/generic/string.h>
#include <util/generic/fwd.h>

// Only to be used internally in tar_archive tests

void AssertEqualArchiveUnittest(const TString& archivePath, const TString& unpackedPath, bool readFiles = true);
