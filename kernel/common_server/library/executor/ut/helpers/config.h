#pragma once

#include <kernel/common_server/library/executor/executor.h>

void ExecutorConfigToString(IOutputStream& os, const TString& storageType, const ui32 threadsCount = 16, bool syncMode = true, const TString& queueTableName = "t000", const TString& storageTableName = "t111");
TTaskExecutorConfig BuildExecutorConfig(const TString& storageType, const ui32 threadsCount = 16, bool syncMode = true, const TString& queueTableName = "t000", const TString& storageTableName = "t111");
