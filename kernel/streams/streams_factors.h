#pragma once

#include <kernel/generated_factors_info/metadata/factors_metadata.pb.h>
#include <kernel/u_tracker/u_tracker.h>

TVector<TString> GetStreamFactorNames(int level);

ui32 GetStreamFactorsSize(int level);

void CalculateStreamFactors(const TUTracker& uTracker, float* dstFeatures, int level);
