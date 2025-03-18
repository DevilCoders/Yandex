#include <yweb/robot/kiwi/protos/kwworm.pb.h>
#include <yweb/robot/kiwi/clientlib/protoreaders.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_reader.cpp>

#include <library/cpp/deprecated/prog_options/prog_options.h>
#include <util/stream/file.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/random/shuffle.h>

typedef TSet< std::pair< TString, std::pair<TString, TString> > > TDiffs;
    // <key, <origin value, new value))

using namespace NJson;

void GetJson(const TString& str, TJsonValue& value) {
    TStringStream in;
    in << str;
    ReadJsonTree(&in, &value);
}

// Here we search only for simple "text" fields
TString GetField(const TJsonValue::TMapType& mm, const TString& name) {
    if (mm.find(name) != mm.end())
        return mm.find(name)->second.GetString();
    return TString();
}

void GetField(const TJsonValue::TMapType& mm, const TString& name, TString& res) {
    res = GetField(mm, name);
}

void GetKiwiResults(TVector<TJsonValue::TMapType>& res, const TString& fileName) {
    TFileInput input(fileName);
    TString splitter = "Key: ";  // TODO: dangerous part, can be changed in the future
    NKiwi::TRecordParser::TOpts stubOptions;
    NKiwi::TProtoTextReader reader(input, splitter, stubOptions);
    NKiwiWorm::TRecord record;
    while (reader.ReadProto(record)) {
        Y_ASSERT(record.TuplesSize() > 0);
        TString s = record.GetTuples(0).GetRawData();
        TJsonValue value;
        GetJson(s, value);
        res.push_back(value.GetMap());
    }
}

void GetStandALoneResults(TVector<TJsonValue::TMapType>& res, const TString& fileName) {
    TFileInput in(fileName);
    TString s;
    while (in.ReadLine(s)) {
        TJsonValue value;
        GetJson(s, value);
        res.push_back(value.GetMap());
    }
}

void GetSitaResults(TVector<TJsonValue::TMapType>& res, const TString& fileName) {
    TFileInput in(fileName);
    TString s;
    while (in.ReadLine(s)) {
        TJsonValue value;
        GetJson(s, value);
        if (!value.IsMap()) {
            Cerr << "Sita's output is not a map...\n";
            Cerr << "\t" << s << Endl;
            continue;
        }

        TJsonValue resultsValue = static_cast<TJsonValue::TMapType>(value.GetMap())["Results"];
        if (!resultsValue.IsArray()) {
            Cerr << "Results are not an array...\n";
            Cerr << "\t" << s << Endl;
            continue;
        }

        TJsonValue::TArray resultsArray = resultsValue.GetArray();
        TJsonValue urlRichContentValue;
        for (size_t i = 0; i < resultsArray.size(); ++i) {
            if (!resultsArray[i].IsMap())
                continue;
            TJsonValue::TMapType vv = resultsArray[i].GetMap();
            TJsonValue::TMapType::const_iterator it = vv.find("UrlRichContentExtractionResult");
            if (it == vv.end()) {
                continue;
            }
            urlRichContentValue = it->second;
        }
        if (urlRichContentValue.IsNull()) {
            Cerr << "UrlRichContentExtractorResult is not presented...\n";
            continue;
        }
        if (!urlRichContentValue.IsMap()) {
            Cerr << "UrlRichContentExtractorResult is not a map...\n";
        }
        TJsonValue::TMapType finalMap = urlRichContentValue.GetMap();
        TJsonValue::TMapType::const_iterator it = finalMap.find("ResultJson");
        if (it == finalMap.end()) {
            Cerr << "There are no ResultJson...\n";
            continue;
        }
        if (!it->second.IsString()) {
            Cerr << "ResultJson is not a string...\n";
            continue;
        }
        {
            TString s = it->second.GetString();
            TJsonValue value;
            GetJson(s, value);
            res.push_back(value.GetMap());
        }
    }
}

void Replace(TString& str) {
    while (1) {
        size_t pos = str.find('\n');
        if (pos == TString::npos)
            break;
        str.replace(pos, 1, "<br>");
    }
}

void OutImage(const TString& imgUrl) {
    Cout << "\t\t<td>";
    if (!imgUrl.empty()) {
        Cout << "<img src=\"" << imgUrl << "\" style=\"width:400px; margin-right: 10px;\">";
    } else {
        Cout << "NO IMAGE";
    }
    Cout << "</td>" << Endl;
}

TJsonValue::TArray GetImages(const TString& jsonImages) {
    TJsonValue value;
    GetJson(jsonImages, value);
    return value.GetArray();
}

void OutImages(const TString& etalonValue, const TString& outputValue) {
    TJsonValue::TArray etalonImages = GetImages(etalonValue);
    TJsonValue::TArray outputImages = GetImages(outputValue);
    /*{
        TJsonValue value;
        GetJson(etalonValue, value);
        etalonImages = value.GetArray();
    }
    {
        TJsonValue value;
        GetJson(outputValue, value);
        outputImages = value.GetArray();
    }*/

    if (etalonImages.empty() && outputImages.empty()) {
        return;
    }

    Cout << "<table border=\"1\">" << Endl;
    size_t lim = Max(etalonImages.size(), outputImages.size());
    for (size_t i = 0; i < lim; ++i) {
        Cout << "\t<tr>" << Endl;
        OutImage(i < etalonImages.size() ? etalonImages[i].GetString(): "");
        OutImage(i < outputImages.size() ? outputImages[i].GetString(): "");
        Cout << "\t</tr>" << Endl;
    }
    Cout << "</table>" << Endl;
}

void HtmlOut(const TString& key, TString etalonValue, TString outputValue) {
    if (etalonValue == outputValue) {
        return;
    }

    Replace(etalonValue);
    Replace(outputValue);
    Cout << "<b>" << key << " before:</b> " << etalonValue << "<br>" << Endl;
    Cout << "<b>" << key << "  after:</b> " << outputValue << "<br>" << Endl;
    if (key == "img") {
        OutImages(etalonValue, outputValue);
    }
}

void GenerateDiff(const TJsonValue::TMapType& v1, const NJson::TJsonValue::TMapType& v2,
                  TSet<TString>& keys, TDiffs& diffs, bool originOrder) {
    for (TJsonValue::TMapType::const_iterator itEtalon = v1.begin(); itEtalon != v1.end(); ++itEtalon) {
        TString key = itEtalon->first;
        if (keys.find(key) != keys.end()) { // we already considered this key
            continue;
        }

        keys.insert(key);
        TString valueEtalon = itEtalon->second.GetStringRobust();
        TJsonValue::TMapType::const_iterator itOutput = v2.find(key);
        TString valueOutput;
        if (itOutput != v2.end()) {
            valueOutput = itOutput->second.GetStringRobust();
        }
        if (valueOutput != valueEtalon) {
            if (originOrder) {
                diffs.insert(std::make_pair(key, std::make_pair(valueEtalon, valueOutput)));
            } else {
                diffs.insert(std::make_pair(key, std::make_pair(valueOutput, valueEtalon)));
            }
        }
    }
}

bool GenerateDiff(const TJsonValue::TMapType& v1, const NJson::TJsonValue::TMapType& v2, int cnt, const TString& url) {
    TSet<TString> keys;
    TDiffs diffs;
    GenerateDiff(v1, v2, keys, diffs, true);
    GenerateDiff(v2, v1, keys, diffs, false);
    if (diffs.empty()) {
        return false;
    }

    Cout << "#" << cnt << ": ";
    Cout << "<a href=\"" << url << "\">" << url << "</a> <br>" << Endl;
    for (TDiffs::const_iterator it = diffs.begin(); it != diffs.end(); ++it) {
        HtmlOut(it->first, it->second.first, it->second.second);
    }
    Cout << "<br>" << Endl;
    return true;
}

void GetHTMLDiff(const TVector<TJsonValue::TMapType>& v1,
                 const TVector<TJsonValue::TMapType>& v2) {
    if (v1.size() != v2.size()) {
        ythrow yexception() << "Different size of diff!";
    }
    Cerr << v1.size() << " pairs will be compared." << Endl;
    Cout << "<meta charset=\"utf-8\">" << Endl;
    Cout << "<title>Diff</title>" << Endl;
    Cout << "<body>" << Endl;

    int cnt = 0;
    for (size_t i = 0; i < v1.size(); ++i) {
        TString url1 = GetField(v1[i], "url");
        TString url2 = GetField(v2[i], "url");
        if (url1 != url2) {
            ythrow yexception() << "Different urls in diff! " << url1 << ' ' << url2 << Endl;
        }

        TString url = GetField(v2[i], "finalurl");
        if (GenerateDiff(v1[i], v2[i], cnt, url)) {
            ++cnt;
        }
    }
    Cout << "</body>" << Endl;
    Cerr << cnt << " pairs are different" << Endl;
}

void FilterAndSort(TVector<TJsonValue::TMapType>& v1,
                   TVector<TJsonValue::TMapType>& v2) {
    TMap<TString, TJsonValue::TMapType> mmap;
    for (size_t i = 0; i < v2.size(); ++i) {
        TString url = GetField(v2[i], "url");
        if (url.empty()) {
            Cerr << "There is no url in ResultJson..." << Endl;
            continue;
        }
        mmap[url] = v2[i];
    }

    TVector<TJsonValue::TMapType> newV1, newV2;
    for (size_t i = 0; i < v1.size(); ++i) {
        TString url = GetField(v1[i], "url");
        if (url.empty()) {
            Cerr << "There is no url in ResultJson..." << Endl;
            continue;
        }
        TMap<TString, TJsonValue::TMapType>::const_iterator it = mmap.find(url);
        if (it != mmap.end()) {
            newV1.push_back(v1[i]);
            newV2.push_back(it->second);
        }
    }

    // we could do v1 = newV1; v2 = newV2; but let's shuffle them!
    TVector<size_t> nums;
    for (size_t i = 0; i < newV1.size(); ++i) {
        nums.push_back(i);
    }
    Shuffle(nums.begin(), nums.end());

    v1.clear();
    v2.clear();
    for (size_t i = 0; i < nums.size(); ++i) {
        v1.push_back(newV1[nums[i]]);
        v2.push_back(newV2[nums[i]]);
    }
}

static void PrintHelp() {
    Cout << "Command line usage is:\n"
        " [-h, -help] - display this help message\n"
        " [-f] <fileName> - origin file of diff\n"
        " [-s] <fileName> - output file of diff\n"
        " [-m] <mode> - mode (sita, standalone or kiwi)\n";
        //" both files are requaired and should be got from rca-standalone\n\n";
}

static int Main(TProgramOptions &progOptions) {
    TString etalonFile(progOptions.GetReqOptVal("f"));
    TString outputFile(progOptions.GetReqOptVal("s"));
    TString mode(progOptions.GetReqOptVal("m"));

    if (mode == "kiwi") {
        TVector<TJsonValue::TMapType> v1, v2;
        GetKiwiResults(v1, etalonFile);
        GetKiwiResults(v2, outputFile);
        FilterAndSort(v1, v2);
        GetHTMLDiff(v1, v2);
    } else if (mode == "standalone") {
        TVector<TJsonValue::TMapType> v1, v2;
        GetStandALoneResults(v1, etalonFile);
        GetStandALoneResults(v2, outputFile);
        FilterAndSort(v1, v2);
        GetHTMLDiff(v1, v2);
    } else if (mode == "sita") {
        TVector<TJsonValue::TMapType> v1, v2;
        GetSitaResults(v1, etalonFile);
        GetSitaResults(v2, outputFile);
        FilterAndSort(v1, v2);
        GetHTMLDiff(v1, v2);
    } else {
        ythrow yexception() << "\t!!! Unsupported mode: " << mode << Endl;
    }
    return 0;
}

int main(int argc, const char* argv[]) {
    TProgramOptions progOptions("|h|help|f|+|s|+|m|+");
    return main_with_options_and_catch(progOptions, argc, argv, Main, PrintHelp);
}

