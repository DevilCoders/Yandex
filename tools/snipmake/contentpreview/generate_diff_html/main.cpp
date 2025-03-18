#include <yweb/robot/kiwi/protos/kwworm.pb.h>
#include <yweb/robot/kiwi/clientlib/protoreaders.h>

#include <library/cpp/archive/yarchive.h>

#include <util/stream/file.h>
#include <util/generic/hash.h>
#include <util/string/builder.h>
#include <util/memory/blob.h>
#include <library/cpp/deprecated/prog_options/prog_options.h>

namespace {
    const unsigned char data[] = {
        #include "data.inc"
    };
};

class TJsonArrayBuilder {
public:
    TJsonArrayBuilder() :
        isBuilded(false)
    {}

    bool Add(const TString& jsonObject) {
        if (isBuilded)
            return false;
        if (diffJsonArray.size() == 0)
            diffJsonArray << "[\n" << jsonObject;
        else
            diffJsonArray << ",\n" << jsonObject;
        return true;
    }

    TString Build() {
        isBuilded = true;
        if (diffJsonArray.size() == 0)
            return "[]";
        return diffJsonArray << "\n]";
    }

private:
    bool isBuilded;
    TStringBuilder diffJsonArray;
};

TString BuildJsonObject(const TString& url, const TString& previewJsons) {
    return TStringBuilder() << "{\"url\":\"" << url << "\",\"preview_jsons\":" << previewJsons << "}";
}

void FillKeyToRawData(NKiwi::TProtoTextReader& reader, THashMap<TString, TString>& keyToRawData) {
    NKiwiWorm::TRecord record;
    while (reader.ReadProto(record)) {
        Y_ASSERT(record.TuplesSize() > 0);
        keyToRawData.insert(std::make_pair(record.GetKey(), record.GetTuples(0).GetRawData()));
    }
}

void FillDiffJsonArrayBuilder(NKiwi::TProtoTextReader& reader, const THashMap<TString, TString>&  keyToRawData, TJsonArrayBuilder& diffJsonArrayBuilder) {
    NKiwiWorm::TRecord record;
    while (reader.ReadProto(record)) {
        Y_ASSERT(record.TuplesSize() > 0);
        TString url = record.GetKey();
        THashMap<TString, TString>::const_iterator it = keyToRawData.find(url);
        TString firstContentPreview = (it != keyToRawData.end())? it->second : "";
        TString secondContentPreview = record.GetTuples(0).GetRawData();
        if (firstContentPreview != secondContentPreview) {
            TJsonArrayBuilder pairBuilder;
            pairBuilder.Add(firstContentPreview);
            pairBuilder.Add(secondContentPreview);
            diffJsonArrayBuilder.Add(BuildJsonObject(url, pairBuilder.Build()));
        }
    }
}

void PrintHelp() {
    Cout << "Command line arguments are: <first_file> <second_file> [-r <result_file>] [-h] [-help]" << Endl
         << "<first_file> <second_file> - results of kiwi queries" << Endl
         << "[-r <result_file>] - a file to put result in, by default is \"result.html\"" << Endl
         << "[-h] [-help] - print this message" << Endl;
}

int GenerateDiffHtml(TProgramOptions& programOptions) {
    const TVector<const char*>& inputFilesNames = programOptions.GetAllUnnamedOptions();

    if (inputFilesNames.size() < 2) {
        PrintHelp();
        return 1;
    }

    TFileInput firstInput(inputFilesNames[0]);
    TFileInput secondInput(inputFilesNames[1]);

    TArchiveReader archive(TBlob::NoCopy(data, sizeof(data)));
    TAutoPtr<IInputStream> firstPartOfRawHtml = archive.ObjectByKey("/diff_first_part.rawhtml");
    TAutoPtr<IInputStream> secondPartOfRawHtml = archive.ObjectByKey("/diff_second_part.rawhtml");

    TString splitter = "Key: ";  // TODO: dangerous part, can be changed in the future
    NKiwi::TRecordParser::TOpts stubOptions;
    NKiwi::TProtoTextReader firstReader(firstInput, splitter, stubOptions);
    NKiwi::TProtoTextReader secondReader(secondInput, splitter, stubOptions);

    THashMap<TString, TString> keyToRawData;
    TJsonArrayBuilder diffJsonArrayBuilder;
    FillKeyToRawData(firstReader, keyToRawData);
    FillDiffJsonArrayBuilder(secondReader, keyToRawData, diffJsonArrayBuilder);

    TString diffJsonArray = diffJsonArrayBuilder.Build();
    TString result = TStringBuilder() << firstPartOfRawHtml->ReadAll() << diffJsonArray
                                     << ";\n" << secondPartOfRawHtml->ReadAll();

    TFixedBufferFileOutput out(programOptions.GetOptVal("r", "result.html"));
    out.Write(result.c_str(), result.size());

    return 0;
}

int main(int argc, const char* argv[]) {
    TProgramOptions programOptions("|h|help|r|?");
    return main_with_options_and_catch(programOptions, argc, argv, GenerateDiffHtml, PrintHelp);
}
