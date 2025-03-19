#pragma once

#include <kernel/generated_factors_info/simple_factors_info.h>

#include <kernel/factors_info/factors_info.h>
#include <kernel/factor_storage/factor_storage.h>

// This header file is auto-generated from factors_gen.in
#include <kernel/web_factors_info/factors_gen.h>

#include <util/generic/fwd.h>

const size_t N_FACTOR_COUNT = FI_FACTOR_COUNT;

const IFactorsInfo* GetWebFactorsInfo();

/////////////////////////////////////////////////
// do not use these methods.
// use GetWebFactorsInfo instead

size_t GetFactorCount();

EFactorId GetFactorIndex(const char* name);
bool GetFactorIndex(const char* name, EFactorId* result);
bool GetFactorIndex(const TString& name, EFactorId* result);

const char* GetFactorName(size_t i);
const char* GetFactorInternalName(size_t i);  // deprecated
const char* const* GetFactorNames();

bool FactorHasTag(size_t index, TFactorInfo::ETag tag);

bool IsOftenZero(size_t i);
bool IsBinary(size_t i);

bool IsStaticFactor(size_t index);
bool IsTextFactor(size_t index);
bool IsLinkFactor(size_t index);
bool IsQueryFactor(size_t index);
bool IsBrowserFactor(size_t index);
bool IsUserFactor(size_t index);
bool IsUnusedFactor(size_t index);
bool IsUnimplementedFactor(size_t index);
bool IsDeprecatedFactor(size_t index);
bool IsRemovedFactor(size_t index);
bool IsNot01Factor(size_t index);
bool IsMetaFactor(size_t index);
bool IsTransFactor(size_t index);
bool IsFastrankFactor(size_t index);
bool IsLocalizedFactor(size_t index);
bool IsStaticReginfoFactor(size_t index);

/////////////////////////////////////////////////

void CutSeoFactors(TFactorView& factors);
void CopyOnlySEOFactors(const TConstFactorView& source, TFactorView& dest);
void CopyOnlySEOFactors(const float* source, float* dest);
