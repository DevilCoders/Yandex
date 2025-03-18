#pragma once

#include <util/generic/ptr.h>
#include <util/stream/input.h>

void GetStaticFile(const TString& key, TString& out);
void GetStaticFile(const TString& key, IOutputStream& out);
