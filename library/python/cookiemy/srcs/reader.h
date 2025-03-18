#pragma once

#include <util/system/types.h>
#include <stddef.h>

namespace cookiemy {

int
getBlock(unsigned char block_id, const char* buf, ui64 buf_size, unsigned int* ids, ui64 ids_size);

int
preParseBlock(const char* cookie, ui64 cookie_size, unsigned char block_id,
         char* buf, ui64 buf_size, ui64 *ids_size);

ui64
getBufferSize(const char* buf, ui64 cookie_size);

} // namespace cookiemy

