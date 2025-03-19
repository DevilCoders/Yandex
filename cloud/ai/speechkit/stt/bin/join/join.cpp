//
// Created by myutman on 11/25/19.
//
// Description of the algorithm: https://wiki.yandex-team.ru/cloud/ai/speechkit/stt/data/join/
//

#include <cloud/ai/speechkit/stt/lib/join/lib.h>

#include <library/cpp/colorizer/colors.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

#include <cstdio>

namespace {
    class TMain: public TMainClassArgs {
        yssize_t Offset_{};
        double Weight_{};
        TString InputFileName_;
        TString InputFormat_;
        TString OutputFileName_;
        TString OutputFormat_;
        TString SplitMeta_;
        TString Distance_;

    protected:
        void RegisterOptions(NLastGetopt::TOpts& opts) override {
            // Brief description fot the whole program, will appear in beginning og help message.
            opts.SetTitle("Join");

            // Built-in options.
            opts.AddHelpOption('h');
            //opts.AddCompletionOption("Utility that joins records bits markups");

            // Custom options.
            opts.AddLongOption("bit_offset")
                .Help("Offset between records bits")
                .RequiredArgument("bit_offset")
                .StoreResult(&Offset_);

            opts.AddLongOption("weight")
                .Help("Weight for likelihood when using Likelihood or LikelihoodWordwise distance")
                .RequiredArgument("weight")
                .DefaultValue(0)
                .StoreResult(&Weight_);

            opts.AddLongOption("split_meta")
                .Help("Json with split meta info")
                .RequiredArgument("split_meta")
                .DefaultValue("")
                .StoreResult(&SplitMeta_);

            opts.AddLongOption("input")
                .Help("Input file with markups")
                .RequiredArgument("input")
                .StoreResult(&InputFileName_);

            opts.AddLongOption("input_format")
                .Help("Format of input")
                .RequiredArgument("format")
                .DefaultValue("txt")
                .StoreResult(&InputFormat_)
                .Completer(
                    NLastGetopt::NComp::Choice(
                        {{"txt", "txt format"},
                         {"json", "json format"}}));

            opts.AddLongOption("output")
                .Help("Output file with markups")
                .RequiredArgument("output")
                .StoreResult(&OutputFileName_);

            opts.AddLongOption("output_format")
                .Help("Format of output")
                .RequiredArgument("output_format")
                .DefaultValue("txt")
                .StoreResult(&OutputFormat_)
                .Completer(
                    NLastGetopt::NComp::Choice(
                        {{"txt", "txt format"},
                         {"json", "json format"}}));

            opts.AddLongOption("distance")
                .Help("Distance function")
                .RequiredArgument("distance")
                .DefaultValue("Levenshtein")
                .StoreResult(&Distance_);
        }

        int DoRun(NLastGetopt::TOptsParseResult&&) override {
            Cerr << InputFileName_ << " " << OutputFileName_ << Endl;
            TVector<TRecordBitsMarkups> recordsBitsMarkups;
            if (InputFormat_ == "txt") {
                recordsBitsMarkups = ReadRecordsBitsMarkupsFromTxt(InputFileName_, Offset_);
            } else if (InputFormat_ == "json") {
                recordsBitsMarkups = ReadRecordsBitsMarkupsFromJson(InputFileName_, Offset_);
            } else {
                throw std::runtime_error("Invalid input format name");
            }
            TVector<TRecordJoin> recordsMarkups;
            if (Distance_ == "Levenshtein") {
                recordsMarkups = JoinRecordsBitsMarkups<ui32>(TLevenshteinDistance(), recordsBitsMarkups);
            } else if (Distance_ == "Wordwise") {
                recordsMarkups = JoinRecordsBitsMarkups<ui32>(TWordwiseLevenshteinDistance(), recordsBitsMarkups);
            } else if (Distance_ == "Likelihood") {
                if (Weight_ >= 0) {
                    recordsMarkups = JoinRecordsBitsMarkups<double>(TLikelihoodLevensteinDistance(Weight_),
                                                                    recordsBitsMarkups);
                } else {
                    recordsMarkups = JoinRecordsBitsMarkups<TLikelihoodValue>(TTupleLikelihoodLevensteinDistance(Weight_),
                                                                              recordsBitsMarkups);
                }
            } else if (Distance_ == "LikelihoodWordwise") {
                if (Weight_ >= 0) {
                    recordsMarkups = JoinRecordsBitsMarkups<double>(TLikelihoodWordwiseLevensteinDistance(Weight_),
                                                                    recordsBitsMarkups);
                } else {
                    recordsMarkups = JoinRecordsBitsMarkups<TLikelihoodValue>(TTupleLikelihoodWordwiseLevensteinDistance(Weight_),
                                                                              recordsBitsMarkups);
                }
            } else {
                throw std::runtime_error("Invalid distance name");
            }

            if (OutputFormat_ == "txt") {
                TOFStream stream(OutputFileName_);
                OutputRecordsMarkupsToOutputStream(recordsMarkups, stream);
            } else if (OutputFormat_ == "json") {
                TIFStream splitMetaFile(SplitMeta_);
                NJson::TJsonValue splitMetaJson;
                NJson::ReadJsonTree(&splitMetaFile, &splitMetaJson);
                TString tag = splitMetaJson["tag"].GetStringRobust();
                TString idTag = splitMetaJson["id_tag"].GetStringRobust();

                TOFStream stream(OutputFileName_);
                OutputRecordsMarkupsAsJson(recordsMarkups, stream, tag, idTag);
            } else {
                throw std::runtime_error("Invalid output format name");
            }
            return 0;
        }
    };
}

int main(int argc, const char** argv) {
    freopen("output.txt", "w", stdout);
    NLastGetopt::NComp::TCustomCompleter::FireCustomCompleter(argc, argv);
    TMain().Run(argc, argv);
    return 0;
}
