#pragma once

#include <kernel/qtree/richrequest/richnode_fwd.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

void GetSnippetHits(TVector<ui32>& sents, TVector<ui32>& masks, const TString& skeyWithFat, const TString& sinvWithFat, const TRichTreeConstPtr& Richtree);
