#pragma once

#include <util/system/defaults.h>
#include <util/generic/string.h>
#include <library/cpp/string_utils/url/url.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/uri/http_url.h>

static const bool CaseSensitiveHashByDefault = true;

int UrlHashValOld(ui64& fnv, const char* path, bool caseSensitive = CaseSensitiveHashByDefault, TString* normPath = nullptr);
int UrlHashVal(ui64& fnv, const char* path, bool caseSensitive = CaseSensitiveHashByDefault, TString* normPath = nullptr);
int UrlHashValCaseInsensitive(ui64& fnv, const char* path, TString* normPath = nullptr);

// UrlHashVal(RewriteUrl2(url)) == UrlHashValOld(url)
void RewriteUrl2New(char* url);

int UrlNormId(const char* url, TString& urlid);

void GetHostCacheName(MD5 &HostSignature, const char *Host, ui8 scheme = THttpURL::SchemeHTTP);
void GetPathCacheName(char Cache[/*FILENAME_MAX*/], MD5 &HostSignature, const char *Path, int sparse = 1);
void GetUrlCacheName(char Cache[/*FILENAME_MAX*/], const char *Url, int sparse = 1);
