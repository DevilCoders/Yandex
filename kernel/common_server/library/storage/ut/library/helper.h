#pragma once

#include <kernel/common_server/library/storage/config.h>

NRTProc::TStorageOptions BuildStorageOptions(const TString& storageType, ui64 cacheLevel = 0, bool consistency = false);
