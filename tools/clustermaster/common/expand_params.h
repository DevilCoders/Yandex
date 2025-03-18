#pragma once

#include "master_list_manager.h"

#include <util/generic/string.h>
#include <util/generic/vector.h>

TVector<TVector<TString> > ExpandParams(
        const TVector<TString> firsts,
        const TVector<TVector<TString> >& paramss,
        const TMasterListManager* listManager);
