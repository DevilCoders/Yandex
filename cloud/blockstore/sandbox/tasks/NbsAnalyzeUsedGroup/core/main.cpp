#include "options.h"
#include "percentile_builder.h"
#include "rate_calculator.h"
#include "ydb_executer.h"

using namespace NCloud::NBlockStore::NAnalyzeUsedGroup;

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    TOptions opt;
    opt.Parse(argc, argv);

    const TDuration ydbTimeTo = TInstant::Now() - TInstant::Seconds(1);
    const TDuration ydbTimeFrom = ydbTimeTo - TDuration::Days(1);

    auto ydbExecutor = std::make_shared<TYDBExecuter>(
        opt.Endpoint,
        opt.Database,
        opt.Token);

    TRateCalculator rateCalculator(
        opt.ThreadCount,
        ydbExecutor,
        ydbTimeFrom,
        ydbTimeTo);

    Cout << TPercentileBuilder::ConverToJson(
        TPercentileBuilder::Build(rateCalculator.GetRateData(opt.Table)));

    ydbExecutor->Stop();

    return 0;
}
