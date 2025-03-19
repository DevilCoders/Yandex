#pragma once

#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

TString GetGradDataName(const TString& target, const TString& oldName);

void AddBackProp(NNeuralNetApplier::TModel& model, const TString& target, const TVector<TString>& from, const NNeuralNetApplier::TWeightsJoinOptions& joinOptions);

bool CheckGrads(NNeuralNetApplier::TModel& model, const TString& target, const TString& from, const TVector<NNeuralNetApplier::TSample>& sample);
