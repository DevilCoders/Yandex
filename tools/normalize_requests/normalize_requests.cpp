#include <dict/recognize/queryrec/queryrecognizer.h>

#include <kernel/geo/utils.h>

#include <search/pumpkin/nearest_query/nearest.h>

#include <ysite/yandex/doppelgangers/normalize.h>
#include <ysite/yandex/reqanalysis/normalize.h>

#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/deprecated/split/delim_string_iter.h>
#include <util/folder/path.h>
#include <util/generic/maybe.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>

enum ENormType {
    E_DOPP,
    E_NORM,
    E_AGG,
    E_PUMPKIN
};


int main(int argc, char** argv) {
    NLastGetopt::TOpts opts;
    opts.AddCharOption('d', NLastGetopt::NO_ARGUMENT, "use doppelgangers normalization");
    opts.AddCharOption('n', NLastGetopt::NO_ARGUMENT, "NormalizeRequest");
    opts.AddCharOption('a', NLastGetopt::NO_ARGUMENT, "NormalizeRequestAggressively");
    opts.AddCharOption('p', NLastGetopt::NO_ARGUMENT, "PumpkinSearch normalization");
    opts.AddCharOption('c', "<size_t> - column of request in tab-separated input (0 - normalize entire line)")
                       .RequiredArgument("size").DefaultValue("0");
    opts.AddCharOption('o', NLastGetopt::NO_ARGUMENT, "keep original request");
    opts.AddCharOption('q', NLastGetopt::NO_ARGUMENT, "ignore errors occurred");

    opts.AddLongOption("rec_lang", "recognize query lang (in dopp norm)").HasArg(NLastGetopt::NO_ARGUMENT);
    opts.AddLongOption("rec_lang_data", "recognize query lang data dir").RequiredArgument("dir");
    opts.AddLongOption("geoa", "path to geoa.c2p for query lang recognizer").RequiredArgument("path");

    opts.AddLongOption("reg", "user region (used in dopp & syn norm lang recognition)").RequiredArgument("region");


    NLastGetopt::TOptsParseResult r(&opts, argc, argv);


    ENormType nt;

    THolder<TDoppelgangersNormalize> doppelgangersNormalize;

    THolder<TQueryRecognizer> qRec;
    TSimpleSharedPtr<NQueryRecognizer::TFactorMill> factorMill;

    TMaybe<TGeoRegion> userReg;


    THolder<TNearestQuerySearcher> pumpkinNormalize;

    if (r.Has('d')) {
        nt = E_DOPP;

        doppelgangersNormalize.Reset(new TDoppelgangersNormalize(true, false, false));

        if (r.Has("rec_lang")) {
            TFsPath recLangDataDir = r.Get("rec_lang_data");
            TString geoaPath = r.Get("geoa");

            TIFStream weightsInput(recLangDataDir / "queryrec.weights");
            factorMill.Reset(new NQueryRecognizer::TFactorMill(TBlob::FromFileContent(recLangDataDir / "queryrec.dict")));
            qRec.Reset(new TQueryRecognizer(factorMill, weightsInput, TGeoTreePtr(new TGeoTree(geoaPath.c_str()))));
        }

        if (r.Has("reg")) {
            userReg = r.Get<TGeoRegion>("reg");
        }
    } else if (r.Has('n')) {
        nt = E_NORM;
    } else if (r.Has('a')) {
        nt = E_AGG;
    } else if (r.Has('p')) {
        nt = E_PUMPKIN;

        pumpkinNormalize.Reset(new TNearestQuerySearcher);
    } else {
        throw yexception() << "No norm specified";
    }

    size_t col = r.GetOrElse<size_t>('c', 0);
    bool keepOrig = r.Has('o', "- keep original request");
    bool quiet = r.Has('q', "- ignore errors occurred");

    TString line; // utf-8
    while (Cin.ReadLine(line)) {
        try {
            TStringStream out;

            TDelimStringIter iter(line, "\t");
            for (size_t i = 1; i < col && iter.Valid(); ++i, ++iter) {
                out << *iter << "\t";
            }
            TString request = (!col) ? line : TString(*iter);
            TString norm;
            switch (nt) {
                case E_DOPP:
                    {
                        if (!!qRec) {
                            TQueryRecognizer::TUserData ud;
                            if (!!userReg)
                                ud.Region = *userReg;

                            TLangMask langMask = qRec->RecognizeParsedQueryLanguage(UTF8ToWide(request), !!userReg ? &ud : nullptr).GetLangMask();
                            norm = doppelgangersNormalize->NormalizeStd(request, langMask);
                        } else {
                            norm = doppelgangersNormalize->NormalizeStd(request);
                        }
                    }
                    break;
                case E_NORM:
                    norm = NormalizeRequestUTF8(request);
                    break;
                case E_AGG:
                    norm = NormalizeRequestAggressively(request);
                    break;
                case E_PUMPKIN:
                    norm = pumpkinNormalize->NormalizeRequest(request);
                    break;
                default:
                    break;
            }
            if (!col) {
                out << norm;
            } else if (iter.Valid()) {
                out << norm;
                if (keepOrig)
                    out << "\t" << iter.GetBegin();
                else
                    out << iter.GetEnd();
            }
            out << Endl;

            Cout << out.Str();
        } catch (...) {
            if (!quiet) {
                throw;
            }
        }
    }
    return 0;
}
