#pragma once

#include <util/generic/fwd.h>
#include <util/stream/output.h>
#include <library/cpp/html/spec/attrs.h>

void ShowAttrs(IOutputStream& os, const char* token, const NHtml::TAttribute* attrs, unsigned n);
TString nicer(const TString& s);
