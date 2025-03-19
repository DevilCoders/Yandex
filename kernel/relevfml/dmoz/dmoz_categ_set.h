#pragma once

#include <util/generic/map.h>
#include <kernel/groupattrs/attrweight.h>

extern void  DmozAddParentThemes(const NGroupingAttrs::TAttrWeights& themes, TMap<int,float>& out);
extern float DmozQueryThemeCombination(const TMap<int,float>& themes);
extern float DmozQueryBestThemeWithMapping(const TMap<int,float>& themes);
