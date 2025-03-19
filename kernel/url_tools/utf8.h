#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#define MAX_UTF8_LEN 4

bool UTF8ToWChar32(const unsigned char* src, const unsigned char* end, wchar32* dst);
bool UTF8ToWChar32Lowercase(const unsigned char* src, const unsigned char* end, wchar32* dst);
bool WChar32ToUTF8(const wchar32* src, unsigned char* dst, unsigned char* end);

size_t CaseInsensitiveSubstUTF8(TString& string, const TStringBuf& from, const TStringBuf& to, bool recursive = false);
