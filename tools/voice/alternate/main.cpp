#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/xrange.h>
#include <util/generic/strbuf.h>
#include <util/stream/file.h>
#include <util/system/yassert.h>

#include <algorithm>


static const TString FIELD_NAME_FORM(TStringBuf("form"));
static const TString FIELD_NAME_LEMMA(TStringBuf("lemma"));
static const TString FIELD_NAME_TRANSCRIPTION(TStringBuf("transcription"));
static const TString FIELD_NAME_NEIGHBOURS(TStringBuf("neighbours"));

static const char FILE_FIELD_DELIMITER = '\t';
static const char SUBST_MATRIX_FIELD_DELIMITER = ' ';
static const char PHONE_DELIMITER = ' ';


class TJoinByFormReducer: public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    enum class ETableIndex: ui32 {
        INFLECTIONS = 0,
        LEXICON
    };

public:
    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TString form = input->GetRow()[FIELD_NAME_FORM].AsString();

        TVector<TString> lemmata;
        THashSet<TString> transcriptions;
        for ( ; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            switch (ETableIndex(input->GetTableIndex())) {
                case ETableIndex::INFLECTIONS:
                    lemmata.push_back(row[FIELD_NAME_LEMMA].AsString());
                    break;
                case ETableIndex::LEXICON:
                    transcriptions.insert(row[FIELD_NAME_TRANSCRIPTION].AsString());
                    break;
                default:
                    Y_FAIL();
            }
        }

        for (const auto& lemma: lemmata) {
            for (const auto& transcription: transcriptions) {
                output->AddRow(
                    NYT::TNode()
                        (FIELD_NAME_LEMMA, lemma)
                        (FIELD_NAME_FORM, form)
                        (FIELD_NAME_TRANSCRIPTION, transcription)
                );
            }
        }
    }
};
REGISTER_REDUCER(TJoinByFormReducer);

class TFindNeighboursReducer: public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
private:
    using TTranscriptionFingerprint = TVector<ui64>;

    struct TFormWithTranscription {
        TString Form;
        TTranscriptionFingerprint TranscriptionFingerprint;

        TFormWithTranscription(const TString& form, const TTranscriptionFingerprint& transcriptionFingerprint)
            : Form(form)
            , TranscriptionFingerprint(transcriptionFingerprint)
        {
        }

        bool operator <(const TFormWithTranscription& other) const {
            if (Form != other.Form) {
                return Form < other.Form;
            }
            return TranscriptionFingerprint < other.TranscriptionFingerprint;
        }
    };

    struct TFormAndDistance {
        TString Form;
        size_t Distance;

        TFormAndDistance(const TString& form, size_t distance)
            : Form(form)
            , Distance(distance)
        {
        }

        bool operator <(const TFormAndDistance& other) const {
            if (Distance != other.Distance) {
                return Distance < other.Distance;
            }
            return Form < other.Form;
        }
    };

public:
    TFindNeighboursReducer() {
    }

    TFindNeighboursReducer(size_t neighbourLimit, bool strictLimit)
        : NeighbourLimit(neighbourLimit)
        , StrictLimit(strictLimit)
    {
    }

    void Save(IOutputStream& stream) const override {
        stream << NeighbourLimit << FILE_FIELD_DELIMITER << size_t(StrictLimit);
    }

    void Load(IInputStream& stream) override {
        stream >> NeighbourLimit;
        size_t strictLimit;
        stream >> strictLimit;
        StrictLimit = bool(strictLimit);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TVector<TFormWithTranscription> formsWithTranscriptions;
        for ( ; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            const TString& form = row[FIELD_NAME_FORM].AsString();
            const TString& transcription = row[FIELD_NAME_TRANSCRIPTION].AsString();

            TTranscriptionFingerprint transcriptionFingerprint;
            for (TStringBuf transcriptionScanner(transcription), phone = transcriptionScanner.NextTok(PHONE_DELIMITER);
                 !transcriptionScanner.empty();
                 phone = transcriptionScanner.NextTok(PHONE_DELIMITER))
            {
                transcriptionFingerprint.push_back(ComputeHash(phone));
            }

            formsWithTranscriptions.emplace_back(form, transcriptionFingerprint);
        }
        Sort(formsWithTranscriptions);

        THashMap<TString, THashMap<TString, size_t>> pairwiseDistances;
        for (auto firstPairIndex: xrange(formsWithTranscriptions.size())) {
            const auto& firstFormWithTranscription = formsWithTranscriptions[firstPairIndex];
            const auto& firstForm = firstFormWithTranscription.Form;
            const auto& firstTranscriptionFingerprint = firstFormWithTranscription.TranscriptionFingerprint;
            auto& distancesFromFirstForm = pairwiseDistances[firstForm];
            for (auto secondPairIndex: xrange(formsWithTranscriptions.size())) {
                const auto& secondFormWithTranscription = formsWithTranscriptions[secondPairIndex];
                const auto& secondForm = secondFormWithTranscription.Form;
                if (secondForm < firstForm) {
                    pairwiseDistances[firstForm][secondForm] = pairwiseDistances[secondForm][firstForm];
                    continue;
                } else if (secondForm == firstForm) {
                    continue;
                }
                const auto& secondTranscriptionFingerprint = secondFormWithTranscription.TranscriptionFingerprint;
                auto distance = NLevenshtein::Distance(
                    firstTranscriptionFingerprint, secondTranscriptionFingerprint
                );
                auto minDistanceIter = distancesFromFirstForm.find(secondForm);
                if (minDistanceIter == distancesFromFirstForm.end()) {
                    distancesFromFirstForm[secondForm] = distance;
                } else {
                    minDistanceIter->second = std::min(minDistanceIter->second, distance);
                }
            }
        }

        for (const auto& formWithNeighbours: pairwiseDistances) {
            const auto& form = formWithNeighbours.first;
            const auto& neighbours = formWithNeighbours.second;

            TVector<TFormAndDistance> potentialNeighbours;
            for (const auto& potentialNeighbourWithDistance: neighbours) {
                const auto& potentialNeighbourForm = potentialNeighbourWithDistance.first;
                const auto distance = potentialNeighbourWithDistance.second;
                if (form != potentialNeighbourForm) {
                    potentialNeighbours.emplace_back(potentialNeighbourForm, distance);
                }
            }
            Sort(potentialNeighbours);

            size_t neighboursToTake = Min(NeighbourLimit, potentialNeighbours.size());
            while (neighboursToTake < potentialNeighbours.size() && !StrictLimit) {
                if (potentialNeighbours[neighboursToTake].Distance ==
                    potentialNeighbours[neighboursToTake - 1].Distance)
                {
                    ++neighboursToTake;
                } else {
                    break;
                }
            }
            auto& chosenNeighbours = potentialNeighbours;

            // Though the resize is only downwards, .resize() doesn't know
            static const size_t INVALID_DISTANCE = std::numeric_limits<size_t>::max();
            chosenNeighbours.resize(neighboursToTake, TFormAndDistance(TString(), INVALID_DISTANCE));

            TVector<TString> neighbourForms;
            for (const auto& neighbour: chosenNeighbours) {
                Y_VERIFY(neighbour.Distance != INVALID_DISTANCE);
                neighbourForms.push_back(neighbour.Form);
            }
            Sort(neighbourForms);
            Y_VERIFY(Unique(neighbourForms.begin(), neighbourForms.end()) == neighbourForms.end());

            NYT::TNode neighbourList = NYT::TNode::CreateList();
            for (const auto& neighbourForm: neighbourForms) {
                neighbourList.Add(neighbourForm);
            }

            output->AddRow(
                NYT::TNode()
                    (FIELD_NAME_FORM, form)
                    (FIELD_NAME_NEIGHBOURS, neighbourList)
            );
        }
    }

private:
    size_t NeighbourLimit = 3;
    bool StrictLimit = false;
};
REGISTER_REDUCER(TFindNeighboursReducer);

int main(int argc, const char* argv[]) {
    NYT::Initialize(argc, argv);

    TString server(TStringBuf("hahn"));
    TString tablePrefix(TStringBuf("//home/voice/kartynnik/alternate"));
    TString inflectionsFile(TStringBuf("inflections.txt"));
    TString lexiconFile(TStringBuf("lexicon"));
    TString outputFile(TStringBuf("neighbours.txt"));
    size_t neighbourLimit = 3;
    bool strictLimit = false;
    size_t reduceJobs = 10000;
    bool scliteMatrixFormat = false;

    {
        NLastGetopt::TOpts options;

        options
            .AddLongOption('s', "server", "YT proxy to use")
            .Optional()
            .RequiredArgument("SERVER")
            .DefaultValue(server)
            .StoreResult(&server);

        options
            .AddLongOption('p', "prefix", "YT table name prefix")
            .Optional()
            .RequiredArgument("YTPATH")
            .DefaultValue(tablePrefix)
            .StoreResult(&tablePrefix);

        options
            .AddLongOption('i', "inflections", "word inflections file")
            .Optional()
            .RequiredArgument("FILE.txt")
            .DefaultValue(inflectionsFile)
            .StoreResult(&inflectionsFile);

        options
            .AddLongOption('l', "lexicon", "form transcriptions file")
            .Optional()
            .RequiredArgument("LEXICON")
            .DefaultValue(lexiconFile)
            .StoreResult(&lexiconFile);

        options
            .AddLongOption('o', "output", "neighbours output file")
            .Optional()
            .RequiredArgument("NEIGHBOURS.txt")
            .DefaultValue(outputFile)
            .StoreResult(&outputFile);

        options
            .AddLongOption('n', "neighbours", "amount of neighbours to consider")
            .Optional()
            .RequiredArgument("NUMBER")
            .DefaultValue(ToString(neighbourLimit))
            .StoreResult(&neighbourLimit);

        // "Fair" inclusion means extending the k-top with the candidates sharing the same distance as the k'th one
        options
            .AddLongOption('S', "strict", "whether to cut neighbours strictly by the limit instead of fair inclusion")
            .Optional()
            .NoArgument()
            .StoreValue(&strictLimit, true);

        options
            .AddLongOption('j', "jobs", "final reduce job count")
            .Optional()
            .RequiredArgument("NUMBER")
            .DefaultValue(ToString(reduceJobs))
            .StoreResult(&reduceJobs);

        options
            .AddLongOption('m', "subst-matrix", "use sclite-py substitution matrix format (one word pair per line)")
            .Optional()
            .NoArgument()
            .StoreValue(&scliteMatrixFormat, true);

        NLastGetopt::TOptsParseResult(&options, argc, argv);
    }


    TString inflectionsTable(tablePrefix + "/inflections");
    TString lexiconTable(tablePrefix + "/lexicon");
    TString tripletsTable(tablePrefix + "/triplets");
    TString resultsTable(tablePrefix + "/results");

    auto client = NYT::CreateClient(server);

    client->Create(
        tablePrefix,
        NYT::NT_MAP,
        NYT::TCreateOptions()
            .Recursive(true)
            .IgnoreExisting(true)
    );

    {
        client->Remove(inflectionsTable, NYT::TRemoveOptions().Force(true));

        TIFStream inflectionsStream(inflectionsFile);
        auto writer = client->CreateTableWriter<NYT::TNode>(inflectionsTable);
        for (TString line; inflectionsStream.ReadLine(line); ) {
            TStringBuf scanner(line);
            TStringBuf lemma = scanner.NextTok(FILE_FIELD_DELIMITER);
            TStringBuf form = scanner.NextTok(FILE_FIELD_DELIMITER);
            Y_VERIFY(scanner.empty());

            writer->AddRow(
                NYT::TNode()
                    (FIELD_NAME_LEMMA, lemma)
                    (FIELD_NAME_FORM, form)
            );
        }
        writer->Finish();

        client->Sort(
            NYT::TSortOperationSpec()
                .AddInput(inflectionsTable)
                .Output(inflectionsTable)
                .SortBy(NYT::TSortColumns(FIELD_NAME_FORM))
        );
    }

    {
        client->Remove(lexiconTable, NYT::TRemoveOptions().Force(true));

        TIFStream lexiconStream(lexiconFile);
        auto writer = client->CreateTableWriter<NYT::TNode>(lexiconTable);
        for (TString line; lexiconStream.ReadLine(line); ) {
            TStringBuf scanner(line);
            TStringBuf form = scanner.NextTok(FILE_FIELD_DELIMITER);
            TStringBuf transcription = scanner.NextTok(FILE_FIELD_DELIMITER);
            Y_VERIFY(scanner.empty());

            writer->AddRow(
                NYT::TNode()
                    (FIELD_NAME_FORM, form)
                    (FIELD_NAME_TRANSCRIPTION, transcription)
            );
        }
        writer->Finish();

        client->Sort(
            NYT::TSortOperationSpec()
                .AddInput(lexiconTable)
                .Output(lexiconTable)
                .SortBy(NYT::TSortColumns(FIELD_NAME_FORM))
        );
    }

    {
        client->Remove(tripletsTable, NYT::TRemoveOptions().Force(true));
        client->JoinReduce(
            NYT::TJoinReduceOperationSpec()
                .AddInput<NYT::TNode>(NYT::TRichYPath(inflectionsTable).Primary(true))
                .AddInput<NYT::TNode>(lexiconTable)
                .AddOutput<NYT::TNode>(tripletsTable)
                .JoinBy(NYT::TSortColumns(FIELD_NAME_FORM)),
            new TJoinByFormReducer()
        );
        client->Sort(
            NYT::TSortOperationSpec()
                .AddInput(tripletsTable)
                .Output(tripletsTable)
                .SortBy(NYT::TSortColumns(FIELD_NAME_LEMMA))
        );
    }

    {
        client->Remove(resultsTable, NYT::TRemoveOptions().Force(true));
        client->Reduce(
            NYT::TReduceOperationSpec()
                .AddInput<NYT::TNode>(tripletsTable)
                .AddOutput<NYT::TNode>(resultsTable)
                .ReduceBy(FIELD_NAME_LEMMA),
            new TFindNeighboursReducer(neighbourLimit, strictLimit),
            NYT::TOperationOptions().Spec(
                NYT::TNode::CreateMap()
                    ("job_count", reduceJobs)
            )
        );
        client->Sort(
            NYT::TSortOperationSpec()
                .AddInput(resultsTable)
                .Output(resultsTable)
                .SortBy(FIELD_NAME_FORM)
        );
    }

    {
        TOFStream output(outputFile);
        auto reader = client->CreateTableReader<NYT::TNode>(resultsTable);
        for ( ; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            const auto& form = row[FIELD_NAME_FORM].AsString();
            if (scliteMatrixFormat) {
                // A substitution pair per line, separated by a space
                for (const auto& neighbour: row[FIELD_NAME_NEIGHBOURS].AsList()) {
                    output << form << SUBST_MATRIX_FIELD_DELIMITER << neighbour.AsString() << Endl;
                }
            } else {
                // A word and all its neigbours in a single line, separated by tabs
                output << form;
                for (const auto& neighbour: row[FIELD_NAME_NEIGHBOURS].AsList()) {
                    output << FILE_FIELD_DELIMITER << neighbour.AsString();
                }
                output << Endl;
            }
        }
    }
}
