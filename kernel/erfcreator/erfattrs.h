#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>
typedef THashMap<TString, TString> TErfAttrs;

void UnpackErfAttrs(const void* data, size_t sz, TErfAttrs& erfAttrs);
