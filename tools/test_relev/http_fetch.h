#pragma once

TString ToKoi8(const TString &szQuery);
TString ToUtf(const TString &szQuery);
TString FetchUrl(const char *szUrl, bool use_xml = false);
