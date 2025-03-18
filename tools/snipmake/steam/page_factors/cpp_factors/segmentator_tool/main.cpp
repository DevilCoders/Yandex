#include <tools/snipmake/steam/page_factors/cpp_factors/page_segment.h>
#include <tools/snipmake/steam/page_factors/cpp_factors/segmentator.h>

#include <yweb/rca/lib/textbuilder/segm_helper.h>
#include <yweb/rca/lib/textbuilder/formatter.h>

#include <kernel/matrixnet/mn_multi_categ.h>
#include <kernel/matrixnet/mn_sse.h>
#include <kernel/recshell/recshell.h>
#include <library/cpp/domtree/builder.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/json_reader.h>

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/charset/codepage.h>
#include <util/folder/dirut.h>
#include <util/folder/filelist.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/string/vector.h>
#include <utility>


using namespace NSegmentator;

static const TMap<ui32, TString> ANNOTATE_CODES_2_CLASSES = {
    {0, "AUX"},
    {1, "AAD"},
    {2, "ACP"},
    {3, "AIN"},
    {4, "ASL"},

    {10, "DMD"},
    {11, "DHC"},
    {12, "DHA"},
    {13, "DCT"},
    {14, "DCM"},

    {20, "LMN"},
    {21, "LCN"},
    {22, "LIN"}
};

static const TMap<TString, ui32> GetAnnotateClasses2Codes() {
    TMap<TString, ui32> annotateClasses2Codes;
    for (const TMap<ui32, TString>::value_type& code2Class : ANNOTATE_CODES_2_CLASSES) {
        annotateClasses2Codes.insert(TMap<TString, ui32>::value_type(code2Class.second,
                                                                    code2Class.first));
    }
    return annotateClasses2Codes;
}


static TString GetMnemonics(ui32 classCode) {
    const TString* mnemonics = ANNOTATE_CODES_2_CLASSES.FindPtr(classCode);
    if (nullptr == mnemonics) {
        ythrow yexception() << "Unknown mnemonics class code: " << classCode;
    }
    return *mnemonics;
}


static void PrintPhaseFactors(const TString& phase, const TString& html,
        const TString& url, const TString& charset)
{
    NDomTree::TDomTreePtr domTreePtr = NDomTree::BuildTree(html, url, charset);
    TSegmentator segmentator(url, domTreePtr);

    if (phase == "annotate") {
        segmentator.DumpAnnotateFactors();
    } else if (phase == "merge") {
        segmentator.DumpMergeFactors();
    } else {
        segmentator.DumpSplitFactors();
    }
}


static void PrintAwarePhaseFactors(const TString& phase, const TString& html,
        const TString& url, const TString& charset, const NJson::TJsonValue& correctSegmentation)
{
    NDomTree::TDomTreePtr domTreePtr = NDomTree::BuildTree(html, url, charset);
    TSegmentator segmentator(url, domTreePtr);

    if (phase == "annotate") {
        segmentator.DumpAwareAnnotateFactors(correctSegmentation, GetAnnotateClasses2Codes());
    } else if (phase == "merge") {
        segmentator.DumpAwareMergeFactors(correctSegmentation);
    } else {
        segmentator.DumpAwareSplitFactors(correctSegmentation);
    }
}


static void ApplyModel(const TString& html, const TString& url, const TString& charset) {
    NDomTree::TDomTreePtr domTreePtr = NDomTree::BuildTree(html, url, charset);
    TSegmentator segmentator(url, domTreePtr);
    const TVector<TPageSegment>& annotateSegments = segmentator.GetOrCalcSegments();

    for (const TPageSegment& seg : annotateSegments) {
        Cout << "Label: " << GetMnemonics(seg.First->TypeId) << Endl;
        seg.PrintPositions();
        Cout << Endl;
        Cout << NKwRichContent::ToUnformattedString(ToZonedString(seg)) << Endl;
    }
}


static void Process(const TString& mode, const TString& phase,
        const TString& html, const TString& url, const TString& charset, const TString& ansJsonStr)
{
    if (mode == "factors") {
        PrintPhaseFactors(phase, html, url, charset);
    } else if (mode == "awareFactors"){
        TStringInput inputJsonStr(ansJsonStr);
        NJson::TJsonValue jsonValue;
        NJson::ReadJsonTree(&inputJsonStr, &jsonValue);
        PrintAwarePhaseFactors(phase, html, url, charset, jsonValue);
    } else {
        ApplyModel(html, url, charset);
    }
}


static void ProcessFile(const TString& inFile, const TString& charset, const TString& dict,
        const TString& mode, const TString& phase, const TString& url, const TString& ansJsonStr = "")
{
    TIFStream htmlStream(inFile);
    TString html = htmlStream.ReadAll();
    TString recognizedCharset = charset;
    if (recognizedCharset.empty()) {
        ECharset code = CODES_UTF8;
        if (!dict.empty()) {
            TRecognizerShell recognizer(dict.data());
            recognizer.RecognizeHtml(html.data(), html.size(), &code, nullptr);
        }
        recognizedCharset = NameByCharset(code);
    }

    Process(mode, phase, html, url, recognizedCharset, ansJsonStr);
}


int main(int argc, char** argv) {
    TString mode("apply");
    TString phase("split");
    TString url("http://127.0.0.1/");
    TString charset;
    TString dict;
    TString inFile;
    TString streamDir;
    TString ansFile;
    size_t ansId = 0;

    NLastGetopt::TOpts opts;
    opts.AddLongOption('m', "mode", "[apply, factors, awareFactors], default = apply").StoreResult(&mode);
    opts.AddLongOption('p', "phase", "phase [split, merge, annotate] for factors mode, default = split").StoreResult(&phase);
    opts.AddLongOption('u', "url", "document url, default = http://127.0.0.1/").StoreResult(&url);
    opts.AddLongOption('c', "charset", "default = utf-8").StoreResult(&charset);
    opts.AddLongOption('d', "dict", "path to 'dict.dict'").StoreResult(&dict);
    opts.AddLongOption('f', "infile", "input file with html").StoreResult(&inFile);
    opts.AddLongOption('s', "streamdir", "dir with input files with html").StoreResult(&streamDir);
    opts.AddLongOption('a', "ansfile", "answers file with STEAM estimations").StoreResult(&ansFile);
    opts.AddLongOption('i', "ansid", "answer id in answers file, default = 0").StoreResult(&ansId);
    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    if (!streamDir.empty()) {
        TFileList fileList;
        fileList.Fill(streamDir, true);
        const char* fname;
        while (nullptr != (fname = fileList.Next())) {
            TString fnameStr(fname);
            Cout << "\t>>>File: " << fnameStr << "<<<" << Endl;
            ProcessFile(streamDir + GetDirectorySeparatorS() + fnameStr, charset, dict, mode, phase, url);
        }
    } else {
        TString ansJsonStr;
        if (mode == "awareFactors") {
            if (ansFile.empty()) {
                ythrow yexception() << "ansfile is needed for " << mode << " mode";
            }
            TIFStream ansStream(ansFile);
            TString ansLine;
            while (ansId > 0 && ansStream.ReadLine(ansLine)) {
                --ansId;
            }
            if (ansId != 0 || !ansStream.ReadLine(ansLine)) {
                ythrow yexception() << "ansfile contains less than ansid lines";
            }
            TVector<TString> splitAnsLine;
            StringSplitter(ansLine).Split('\t').SkipEmpty().Limit(3).Collect(&splitAnsLine);
            if (splitAnsLine.size() != 3) {
                ythrow yexception() << "invalid ansfile format";
            }
            ansJsonStr = splitAnsLine[2];
        }

        ProcessFile(inFile, charset, dict, mode, phase, url, ansJsonStr);
    }
}
