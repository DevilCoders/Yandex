#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>
#include <util/stream/file.h>


static const TString PHRASE_FIELD(TStringBuf("key"));


class TFilteringMapper: public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    typedef THashSet<TString> TVocabulary;

public:
    TFilteringMapper() {
    }

    TFilteringMapper(const TVocabulary& vocabulary)
        : Vocabulary(vocabulary)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TString phrase = input->GetRow()[PHRASE_FIELD].AsString();
        for (TStringBuf restPhrase(phrase), token = restPhrase.NextTok(' ');
            !(token.empty() && restPhrase.empty());
            token = restPhrase.NextTok(' '))
        {
            if (Vocabulary.contains(TString{token})) {
                output->AddRow(input->GetRow());
                break;
            }
        }
    }

    void Save(IOutputStream& stream) const override {
        ::Save(&stream, Vocabulary);
    }

    void Load(IInputStream& stream) override {
        ::Load(&stream, Vocabulary);
    }

private:
    TVocabulary Vocabulary;
};
REGISTER_MAPPER(TFilteringMapper);


int main(int argc, const char* argv[]) {
    NYT::Initialize(argc, argv);

    TString server(TStringBuf("hahn"));
    TString inputTable(TStringBuf("//home/voice/kolesov93/corpus"));
    TString outputTable(TStringBuf("//home/voice/kolesov93/pruned-corpus"));
    TString vocabularyFile(TStringBuf("words.txt"));

    {
        NLastGetopt::TOpts options;

        options
            .AddLongOption('s', "server", "YT proxy to use")
            .Optional()
            .RequiredArgument("SERVER")
            .DefaultValue(server)
            .StoreResult(&server);

        options
            .AddLongOption('i', "input", "input YT table name")
            .Optional()
            .RequiredArgument("YTPATH")
            .DefaultValue(inputTable)
            .StoreResult(&inputTable);

        options
            .AddLongOption('o', "output", "output YT table name")
            .Optional()
            .RequiredArgument("YTPATH")
            .DefaultValue(outputTable)
            .StoreResult(&outputTable);

        options
            .AddLongOption('w', "words", "word vocabulary file")
            .Optional()
            .RequiredArgument("words.txt")
            .DefaultValue(vocabularyFile)
            .StoreResult(&vocabularyFile);

        NLastGetopt::TOptsParseResult(&options, argc, argv);
    }

    TFilteringMapper::TVocabulary vocabulary;
    {
        TIFStream vocabularyStream(vocabularyFile);
        for (TString line; vocabularyStream.ReadLine(line); ) {
            vocabulary.insert(line);
        }
    }

    auto client = NYT::CreateClient(server);

    {
        client->Map(
            NYT::TMapOperationSpec()
                .AddInput<NYT::TNode>(inputTable)
                .AddOutput<NYT::TNode>(outputTable),
            new TFilteringMapper(vocabulary)
        );
    }
}
