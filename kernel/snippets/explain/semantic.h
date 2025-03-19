#pragma once

#include <kernel/tarc/docdescr/docdescr.h>

struct IInfoDataTable;

namespace NSnippets {

bool HasSemanticMarkup(const TDocInfos& docInfos);
void PrintSemanticMarkup(const TDocInfos& docInfos, IInfoDataTable* result);

}
