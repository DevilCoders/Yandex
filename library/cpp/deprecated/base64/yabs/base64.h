#pragma once

#include <library/cpp/string_utils/base64/base64.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <stddef.h>
#include <sys/types.h>

// NOTE: These functions are deprecated, incompatible with standard base64
// and useless outside of yabs/server.
//
// All new code (except when using encoded data as part of URL of a yabs'
// handler) should use <library/cpp/string_utils/base64/base64.h>.

// Get size of encoded data.
constexpr size_t GetYabsSizeBase64Encoded(const size_t sizeBase64Decoded, bool pad) {
    return pad
         ? (sizeBase64Decoded + 2) / 3 * 4
         : (sizeBase64Decoded * 4 + 2) / 3;
}

// Get size of encoded data that will fit in the buffer
// of the provided size when decoded.
constexpr size_t GetYabsSizeBase64EncodedFit(const size_t sizeBase64Decoded) {
    return (sizeBase64Decoded * 4 + 2) / 3;
}

// Get size of decoded data.
constexpr size_t GetYabsSizeBase64Decoded(const size_t sizeBase64Encoded) {
    return sizeBase64Encoded * 3 / 4;
}

// Get size of decoded data that will fit in the buffer
// of the provided size when encoded.
constexpr size_t GetYabsSizeBase64DecodedFit(const size_t sizeBase64Encoded, bool pad) {
    return pad
         ? sizeBase64Encoded / 4 * 3
         : sizeBase64Encoded * 3 / 4;
}

size_t YabsBase64Encode(unsigned char const* buf, size_t buf_size, char* str, size_t str_size);
size_t YabsSpylogBase64Encode(unsigned char const* buf, size_t buf_size, char* str, size_t str_size);
size_t YabsMarketBase64Encode(unsigned char const* buf, size_t buf_size, char* str, size_t str_size);
size_t YabsRfcBase64Encode(unsigned char const* buf, size_t buf_size, char* str, size_t str_size);
TString YabsBase64Decode(TStringBuf const& buf);
TString YabsBase64Encode(TStringBuf const& buf);
TString YabsBase64Encode(unsigned char const* buf, size_t buf_size);
TString YabsRfcBase64Decode(TStringBuf const& buf);
TString YabsRfcBase64Encode(TStringBuf const& buf);
ssize_t YabsBase64Decode(char const* str, size_t str_size, unsigned char* buf, size_t buf_size);
ssize_t YabsBase64Decode(char const* str, size_t str_size, unsigned char* buf, size_t buf_size, bool& fail);
ssize_t YabsRfcBase64Decode(char const* str, size_t str_size, unsigned char* buf, size_t buf_size);
ssize_t YabsRfcBase64Decode(char const* str, size_t str_size, unsigned char* buf, size_t buf_size, bool& fail);
