/* tool to debug tools/dolbilo/libs/rps_schedule/rpsschedule.h stepper */

#include <tools/dolbilo/libs/rps_schedule/rpsschedule.h>
#include <library/cpp/getopt/last_getopt.h>

struct TOpts {
    inline TOpts(int argc, char** argv)
        : BatterySize(1)
        , GuncrewNum(0)
    {
        NLastGetopt::TOpts opts;
        TString modeStr;

        opts.SetTitle("===========================================================\n"
                      "See https://wiki.yandex-team.ru/JandeksPoisk/Sepe/Dolbilka/\n"
                      "===========================================================\n");
        opts.AddHelpOption('?');
        opts.AddLongOption('\0', "rps-schedule", "Use yandex-tank rps schedule.")
            .RequiredArgument("SCHEDULE")
            .Optional()
            .StoreResult(&RpsSchedule);
        opts.AddLongOption('\0', "battery", "Battery size (send only every NUM-th request in rps-schedule mode).")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&BatterySize);
        opts.AddLongOption('\0', "guncrew", "Guncrew serial within `battery` (start with NUM-th request).")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&GuncrewNum);
        opts.AddLongOption('\0', "start", "Start at UNIXTIME.")
            .RequiredArgument("UNIXTIME")
            .Optional()
            .StoreResult(&Start);
        opts.SetFreeArgsMax(0);

        const NLastGetopt::TOptsParseResult optsres(&opts, argc, argv);
        try {
            if (!(/* 0 <= GuncrewNum && */ GuncrewNum < BatterySize)) {
                ythrow NLastGetopt::TUsageException() << "battery size must be positive, guncrew serial must be within [0; battery)";
            }
        } catch (...) {
            optsres.HandleError();
        }
    }

    TRpsSchedule RpsSchedule;
    unsigned int BatterySize;
    unsigned int GuncrewNum;
    TDuration Start;
};

int main(int argc, char** argv) {
    try {
        const TOpts opts(argc, argv);

        TInstant now = TInstant::Zero() + opts.Start;
        TRpsScheduleIterator it(opts.RpsSchedule, now);

        if (opts.RpsSchedule[0].Mode == SM_AT_UNIX)
            now = it.NextShot(now); // synchronize start

        now = it.NextShot(now, opts.GuncrewNum); // synchronize delay

        while (now != TInstant::Zero()) {
            Cout << now.MicroSeconds() << "\n";
            now = it.NextShot(now, opts.BatterySize);
        }

        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
    return 1;
}
