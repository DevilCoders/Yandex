#include <kernel/dups/banned_info_reader.h>
#include <kernel/dups/banned_info/protos/banned_info.pb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <util/stream/str.h>

using namespace NDups::NBannedInfo;

struct TForceFail: public yexception {};

struct TProgramParameters {
    TVector<TString> InputFiles;
    TString OutputFile;
    bool SkipTrivialGroups = false;
    bool Werror = false;
    bool TrackRepetitions = false;
};

class TMain {
private:
    TProgramParameters ProgramParameters;

private:
    TMap<TString, THolder<TBannedInfoListContent>> ContentDict;
    TBannedInfoArchive Archive;

public:
    TMain(int argc, const char* argv[]) {
        using namespace NLastGetopt;
        TOpts opts = NLastGetopt::TOpts::Default();
        opts.AddHelpOption();
        opts.AddVersionOption();
        opts.AddLongOption("output", "output serialized proto message file name").Required().StoreResult(&ProgramParameters.OutputFile);
        opts.AddLongOption("skip-trivial-groups", "ignore group if it matches to single hash value").NoArgument().SetFlag(&ProgramParameters.SkipTrivialGroups);
        opts.AddLongOption("track-repetitions", "search for equivalent sets of hashes").NoArgument().SetFlag(&ProgramParameters.TrackRepetitions);
        opts.AddLongOption("Werror", "Threat warnings as errors").NoArgument().SetFlag(&ProgramParameters.Werror);
        opts.SetFreeArgsMin(1);
        opts.SetFreeArgDefaultTitle("TSV", "input tab separated file with duplicates");
        TOptsParseResult res(&opts, argc, argv);
        ProgramParameters.InputFiles = res.GetFreeArgs();
    }

    int Run() {
        CheckParams();
        LoadInput();
        PackToArchive();
        Save();
        return 0;
    }

private:
    void CheckParams() const {
        Y_ENSURE(ProgramParameters.InputFiles.size() >= 1);
    }

    void LoadInput() {
        TImportParameters params;
        params.EarlyValidation = true;
        params.SkipTrivialGroups = ProgramParameters.SkipTrivialGroups;
        params.TrackContentRepeats = ProgramParameters.TrackRepetitions;

        for (const TString& inputFileName : ProgramParameters.InputFiles) {
            TFsPath path{inputFileName};
            path.CheckExists();
            Y_ENSURE(path.IsFile(), "Not a file: " << inputFileName);
            const TString key = path.Basename();
            Y_ENSURE(!ContentDict.contains(key), "Duplicate file name: " << key);

            TFileInput inputStream{inputFileName};
            TTSVImportResult import = NDups::NBannedInfo::ImportFromTSVStream(inputStream, params);
            for (const auto& line : import.Log) {
                Cerr << "[" << key << "]" << line << Endl;
            }

            Y_ENSURE(import.Content, "Unable to parse file " << key);
            if (ProgramParameters.Werror && import.WarningsNum != 0) {
                ythrow TForceFail() << "Werror mode is active; warnings = " << import.WarningsNum;
            }
            ContentDict[key] = std::move(import.Content);
        }
    }

    void PackToArchive() {
        Archive.Clear();
        auto* map = Archive.MutableNamedList();
        for (auto& item : ContentDict) {
            const TString key = item.first;
            auto contentHolder = std::move(item.second);
            Y_ENSURE(map->count(key) == 0, "Duplicate key: " << key);
            Y_ENSURE(contentHolder != nullptr);

            (*map)[key] = std::move(*contentHolder);
        }
        ContentDict.clear();
    }

    void Save() {
        auto outputStream = OpenOutput(ProgramParameters.OutputFile);
        Y_ENSURE(Archive.SerializeToArcadiaStream(outputStream.Get()));
        outputStream->Finish();
    }
};

int main(int argc, const char* argv[]) {
    try {
        return TMain(argc, argv).Run();
    } catch (TForceFail) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    } catch (...) {
        Cerr << "Uncaught exception:\n"
             << CurrentExceptionMessage() << Endl;
        throw;
    }
}
