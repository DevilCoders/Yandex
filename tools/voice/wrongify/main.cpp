#include <mapreduce/yt/interface/client.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/getopt/last_getopt.h>


static const TString FIELD_NAME_UTTERANCE_ID(TStringBuf("utterance_id"));
static const TString FIELD_NAME_POSITION(TStringBuf("position"));
static const TString FIELD_NAME_PHRASE(TStringBuf("phrase"));
static const TString FIELD_NAME_FORM(TStringBuf("form"));
static const TString FIELD_NAME_NEIGHBOURS(TStringBuf("neighbours"));
static const TString FIELD_NAME_VARIANT(TStringBuf("variant"));

static const char WORD_DELIMITER = ' ';
/*
static const char UTTERANCE_ID_START = '(';
static const char UTTERANCE_ID_END = ')';
static const char UTTERANCE_POSITION_SEPARATOR = '-';
static const char UTTERANCE_VARIANT_SEPARATOR = '-';
*/

static const size_t DATA_SIZE_PER_JOB = 64 * 1024 * 1024;


class TFindNeighboursReducer: public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    enum class ETableIndex: ui32 {
        NEIGHBOURS = 0,
        PHRASE_WORDS = 1
    };
public:
    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        NYT::TNode neighbours = NYT::TNode::CreateList();
        if (ETableIndex(input->GetTableIndex()) == ETableIndex::NEIGHBOURS) {
            neighbours = input->GetRow()[FIELD_NAME_NEIGHBOURS];
            input->Next();
        }
        for ( ; input->IsValid(); input->Next()) {
            // Y_VERIFY(ETableIndex(input->GetTableIndex()) == ETableIndex::PHRASE_WORDS);
            if (ETableIndex(input->GetTableIndex()) != ETableIndex::PHRASE_WORDS) {
                continue;
            }
            NYT::TNode row = input->GetRow();
            row[FIELD_NAME_NEIGHBOURS] = neighbours;
            output->AddRow(row);
        }
    }
};
REGISTER_REDUCER(TFindNeighboursReducer);


class TWrongifyReducer: public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TString utteranceId = input->GetRow()[FIELD_NAME_UTTERANCE_ID].AsString();

        TString previousWords;
        for ( ; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();

            const auto& position = row[FIELD_NAME_POSITION].AsUint64();
            const auto& form = row[FIELD_NAME_FORM].AsString();
            const auto& neighbours = row[FIELD_NAME_NEIGHBOURS];

            size_t variant = 0;
            for (const auto& neighbour: neighbours.AsList()) {
                const auto& wronglyFinishedPhrase = previousWords + neighbour.AsString();
                output->AddRow(
                    NYT::TNode()
                        (FIELD_NAME_UTTERANCE_ID, utteranceId)
                        (FIELD_NAME_POSITION, position)
                        (FIELD_NAME_VARIANT, variant)
                        (FIELD_NAME_PHRASE, wronglyFinishedPhrase)
                );

                ++variant;
            }

            previousWords += form;
            previousWords.push_back(WORD_DELIMITER);
        }
    }
};
REGISTER_REDUCER(TWrongifyReducer);


int main(int argc, const char* argv[]) {
    NYT::Initialize(argc, argv);

    TString server(TStringBuf("hahn"));
    TString tablePrefix(TStringBuf("//home/voice/kartynnik/alternate"));
    size_t reduceJobs = 10000;
    TString inputFile(TStringBuf("/dev/stdin"));
    TString outputFile(TStringBuf("/dev/stdout"));

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
            .AddLongOption('i', "input", "input texts file")
            .Optional()
            .RequiredArgument("sentences.txt")
            .DefaultValue(inputFile)
            .StoreResult(&inputFile);

        options
            .AddLongOption('o', "output", "output wrong-last-word transcritions file")
            .Optional()
            .RequiredArgument("transcriptions.wlw")
            .DefaultValue(outputFile)
            .StoreResult(&outputFile);

        options
            .AddLongOption('j', "jobs", "reduce job count")
            .Optional()
            .RequiredArgument("NUMBER")
            .DefaultValue(ToString(reduceJobs))
            .StoreResult(&reduceJobs);

        NLastGetopt::TOptsParseResult(&options, argc, argv);
    }


    TString neighboursTable(tablePrefix + "/results");
    TString phraseWordsTable(tablePrefix + "/phrase-words");
    TString matchedNeighboursTable(tablePrefix + "/matched-neighbours");
    TString wrongifiedPhrasesTable(tablePrefix + "/wrongified-phrases");

    auto client = NYT::CreateClient(server);

    client->Create(
        tablePrefix,
        NYT::NT_MAP,
        NYT::TCreateOptions()
            .Recursive(true)
            .IgnoreExisting(true)
    );

    {
        client->Remove(phraseWordsTable, NYT::TRemoveOptions().Force(true));

        TIFStream phrasesStream(inputFile);
        auto writer = client->CreateTableWriter<NYT::TNode>(phraseWordsTable);
        for (TString line; phrasesStream.ReadLine(line); ) {
            /*
            TStringBuf scanner(line);
            TStringBuf phrase = scanner.NextTok(UTTERANCE_ID_START);
            TStringBuf utteranceId = scanner.NextTok(UTTERANCE_ID_END);
            Y_VERIFY(scanner.Empty());
            */
            TStringBuf phrase(line);
            const auto& utteranceId = MD5::Calc(phrase);

            size_t position = 0;
            for (TStringBuf word = phrase.NextTok(WORD_DELIMITER);
                 !word.empty();
                 word = phrase.NextTok(WORD_DELIMITER))
            {
                writer->AddRow(
                    NYT::TNode()
                        (FIELD_NAME_FORM, word)
                        (FIELD_NAME_UTTERANCE_ID, utteranceId)
                        (FIELD_NAME_POSITION, position)
                );

                ++position;
            }
        }
        writer->Finish();

        client->Sort(
            NYT::TSortOperationSpec()
                .AddInput(phraseWordsTable)
                .Output(phraseWordsTable)
                .SortBy(NYT::TSortColumns(FIELD_NAME_FORM, FIELD_NAME_UTTERANCE_ID, FIELD_NAME_POSITION))
        );
    }

    {
        client->Remove(matchedNeighboursTable, NYT::TRemoveOptions().Force(true));
        client->Reduce(
            NYT::TReduceOperationSpec()
                .AddInput<NYT::TNode>(neighboursTable)
                .AddInput<NYT::TNode>(phraseWordsTable)
                .AddOutput<NYT::TNode>(matchedNeighboursTable)
                .ReduceBy(NYT::TSortColumns(FIELD_NAME_FORM)),
            new TFindNeighboursReducer(),
            NYT::TOperationOptions().Spec(
                NYT::TNode::CreateMap()
                    ("job_count", reduceJobs)
            )
        );
        client->Sort(
            NYT::TSortOperationSpec()
                .AddInput(matchedNeighboursTable)
                .Output(matchedNeighboursTable)
                .SortBy(NYT::TSortColumns(FIELD_NAME_UTTERANCE_ID, FIELD_NAME_POSITION))
        );
    }

    {
        client->Remove(wrongifiedPhrasesTable, NYT::TRemoveOptions().Force(true));
        client->Reduce(
            NYT::TReduceOperationSpec()
                .AddInput<NYT::TNode>(matchedNeighboursTable)
                .AddOutput<NYT::TNode>(wrongifiedPhrasesTable)
                .ReduceBy(NYT::TSortColumns(FIELD_NAME_UTTERANCE_ID))
                .SortBy(NYT::TSortColumns(FIELD_NAME_UTTERANCE_ID, FIELD_NAME_POSITION)),
            new TWrongifyReducer(),
            NYT::TOperationOptions().Spec(
                NYT::TNode::CreateMap()
                    ("job_count", reduceJobs)
                    ("data_size_per_job", DATA_SIZE_PER_JOB)
            )
        );
        client->Sort(
            NYT::TSortOperationSpec()
                .AddInput(wrongifiedPhrasesTable)
                .Output(wrongifiedPhrasesTable)
                .SortBy(NYT::TSortColumns(FIELD_NAME_UTTERANCE_ID, FIELD_NAME_POSITION, FIELD_NAME_VARIANT))
        );
    }

    {
        TOFStream output(outputFile);
        auto reader = client->CreateTableReader<NYT::TNode>(wrongifiedPhrasesTable);
        for ( ; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            /*
            const auto& utteranceId = row[FIELD_NAME_UTTERANCE_ID].AsString();
            const auto& position = row[FIELD_NAME_POSITION].AsUint64();
            const auto& variant = row[FIELD_NAME_VARIANT].AsUint64();
            */
            const auto& phrase = row[FIELD_NAME_PHRASE].AsString();
            output << phrase
                /*
                << WORD_DELIMITER << UTTERANCE_ID_START
                << utteranceId
                << UTTERANCE_POSITION_SEPARATOR << position
                << UTTERANCE_VARIANT_SEPARATOR << variant
                << UTTERANCE_ID_END
                */
                << Endl;
        }
    }
}
