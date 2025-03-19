#pragma once
#include <kernel/facts/dynamic_list_replacer/dynamic_list_replacer.h>

namespace NFacts {

NSc::TValue FindAuthorByComment(const NSc::TArray &authors, const TStringBuf &factText, size_t maxCommentLength, size_t minAllowedCommonSliceSize);

}  // namespace NFacts
