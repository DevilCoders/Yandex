#pragma once

#include <kernel/qtree/richrequest/richnode.h>


bool IsFullSubtraction(const TRichNodePtr& root);
bool HasUserOperators(const TRichNodePtr& root);
bool HasUrlOperators(const TRichNodePtr& root);
