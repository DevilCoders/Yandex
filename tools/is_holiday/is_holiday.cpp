#include <kernel/is_holiday/is_holiday.h>
#include <library/cpp/deprecated/split/delim_string_iter.h>
#include <library/cpp/deprecated/prog_options/prog_options.h>

#include <util/draft/date.h>

int is_holiday(TProgramOptions& progOptions)
{
    TRegionsDB regionsDB(progOptions.GetReqOptVal("geodata"));
    THolidayChecker hc(progOptions.GetReqOptVal("holidays_spec"), &regionsDB);

    TString l;
    while (Cin.ReadLine(l)) {
        TDelimStringIter it(l,"\t");
        TDate date;
        it.Next(date);
        TGeoRegion reg = 225;
        it.TryNext(reg);
        Cout << hc.IsHoliday(date, reg) << Endl;
    }
    return 0;
}

void PrintHelp()
{
    Cout << "is_holiday <opts>\n"
        " input: <yyyymmdd>[\t reg], if no reg specified it's 225 by default\n"
        " output: <flag>, is _holiday: 1 - true, 0 - false\n"
        " options:\n"
        " -geodata <file>           - path to geodata3.bin\n"
        " -holidays_spec <file>     - file with holidays spec\n";
}

int main(int argc, const char* argv[])
{
    TProgramOptions progOptions("|geodata|+|holidays_spec|+");
    return main_with_options_and_catch(progOptions, argc, argv, is_holiday, PrintHelp);
}




