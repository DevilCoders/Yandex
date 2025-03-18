#pragma once

#include "classificator.h"
#include "req_types.h"
#include "stat.h"

#include <antirobot/lib/stats_writer.h>

#include <util/draft/matrix.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <utility>

class IOutputStream;

namespace NAntiRobot {

class TRequest;
struct TRequestContext;
class TProcessorLinearizedFactors;
class TCacherLinearizedFactors;

using TExpDescriptions = TVector<std::pair<TString, float>>;
using TFormulasResults = TVector<std::pair<TString, float>>;

template<class TLinearizedFactors>
class TClassifyExperiments {
public:
    explicit TClassifyExperiments(const TExpDescriptions& expDescriptions);
    ~TClassifyExperiments();
    TFormulasResults Apply(const TRequest& req, const TLinearizedFactors& lf);
    void PrintStats(TStatsWriter& out) const;
private:
    class TImpl;
    THolder<TImpl> Impl;
};

} // namespace NAntiRobot
