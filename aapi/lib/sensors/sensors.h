#pragma once


#include <util/generic/string.h>
#include <util/datetime/base.h>

namespace NAapi {

void InitSolomonSensors(const TString& host, ui64 port);

void InitSensor(const TString& sensor, i64 value = 0, bool modeDeriv = false);

void SetSensor(const TString& sensor, i64 value);

void AddToSensor(const TString& sensor, i64 add);

}
