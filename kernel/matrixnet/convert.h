#pragma once

#include "mn_trees.h"
#include "mn_sse.h"
#include "mn_dynamic.h"

namespace NMatrixnet {

void MnConvert(const TMnSseInfo    &from, TMnTrees &to);
void MnConvert(const TMnSseDynamic &from, TMnTrees &to);

}

