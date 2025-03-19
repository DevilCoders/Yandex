#pragma once

#include <kernel/porno/proto/ad_cat.pb.h>

#include <util/generic/fwd.h>

bool IsValidAdCatCategoryName(const TStringBuf category);
bool HasDefaultValue(const TAdCatProto& adCat);

bool SetAdCatCategory(const EAdCat category, TAdCatProto& adCat);

TString AdCatToString(const TAdCatProto& categories);
bool TryAdCatFromString(const TStringBuf categories, TAdCatProto& adCat);
TAdCatProto AdCatFromString(const TStringBuf categories);
