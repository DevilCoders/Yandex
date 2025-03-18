#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/singleton.h>

char* EncodeRFC1738(char* buf, size_t len, const char* url, size_t urllen) noexcept;

//! converts an URL from windows-1251 to utf-8 and encodes it according to RFC1738 (see URL Character Encoding)
//! BEWARE: also cuts the last slash (if any)!!!
//! Always consider using NUrlNorm::NormalizeUrl instead.
//! @param pszRes       destination buffer
//! @param nResBufSize  size of buffer
//! @param pszUrl       URL to be normalized
//! @return @c false if buffer is not long enough to contain normalized URL
bool NormalizeUrl(char* pszRes, size_t nResBufSize, const char* pszUrl, size_t pszUrlSize = TString::npos);

//! converts an URL from windows-1251 to utf-8 and encodes it according to RFC1738 (see URL Character Encoding)
//! BEWARE: also cuts the last slash (if any)!!!
//! Always consider using NUrlNorm::NormalizeUrl instead.
//! @param url          URL to be normalized
//! @return normalized URL
//! @throw yexception if the URL cannot be normalized
TString NormalizeUrl(const TStringBuf& url);

TUtf16String NormalizeUrl(const TWtringBuf& url);

TString StrongNormalizeUrl(const TStringBuf& url);
