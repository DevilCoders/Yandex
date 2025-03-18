#include "config.h"

#include <robot/deprecated/gemini/mr/libmrutils/multilang_urls.h>
#include <yweb/robot/kiwi/protos/kwworm.pb.h>

#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/zlib.h>
#include <util/stream/buffer.h>
#include <util/string/vector.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/messagext.h>

#include <library/cpp/http/fetch/httpheader.h>
#include <library/cpp/http/fetch/httpzreader.h>
#include <util/string/split.h>



static bool DocContentsFromHttpResponse(
    const void* httpRespData,
    size_t httpRespSize,
    IOutputStream& out,
    bool uncompress)
{
    const void *response = httpRespData;
    size_t respSize = httpRespSize;

    TBuffer buffer(uncompress ? 1 << 20 : 0);

    if (uncompress) {
        TMemoryInput memIn((const char *)httpRespData, httpRespSize);
        TZLibDecompress decompIn(&memIn);
        TBufferOutput decompOut(buffer);
        respSize = TransferData(&decompIn, &decompOut);
        response = buffer.Data();
    }

    THttpHeader header;
    SCompressedHttpReader<TMemoReader> httpReader;
    httpReader.TMemoReader::Init((void *)response, respSize);
    if (0 != httpReader.Init(&header, 1)) {
        return false;
    }

    while (true) {
        void* buf = nullptr;
        long sz = httpReader.Read(buf);
        if (sz > 0)
            out.Write(buf, sz);
        else if (sz == 0)
            break;
        else
            return false;
    }

    return true;
}

TString UnpackOriginalDoc(const TString &url, const TString& originalDoc) {
    TString result;
    TBuffer buf;
    TBufferOutput out(buf);
    bool uncompress = true;
    bool success = DocContentsFromHttpResponse(originalDoc.data(), originalDoc.size(), out, uncompress);

    if (success) {
        buf.AsString(result);
    }
    else {
        Cerr << "Bad original doc for url: " << url << Endl;
    }

    return result;
}

void FormatHostDump(const TFormatHostDumpConfig &config) {
    const ui32 MAX_PATH_LENGTH = 255;
    TString rawData = Cin.ReadAll();
    NKiwiWorm::TRecord kiwiRecord;
    ui32 filesNumber = 0;
    TString originalDoc;
    TString rootDir = config.RootDir;
    TMemoryInput inputStream(rawData.data(), rawData.size());
    google::protobuf::io::TCopyingInputStreamAdaptor adaptor(&inputStream);

    while (google::protobuf::io::ParseFromZeroCopyStreamSeq(&kiwiRecord, &adaptor)) {
        TString url = kiwiRecord.GetKey();

        for (size_t i = 0; i < kiwiRecord.TuplesSize(); ++i) {
            TString attributeName;
            TString attributeValue;

            if (kiwiRecord.GetTuples(i).HasAttrName()) {
                attributeName = kiwiRecord.GetTuples(i).GetAttrName();
            } else {
                ythrow yexception() << i << "-th tuple in kiwi record have no attr name and id.";
            }

            if (kiwiRecord.GetTuples(i).HasStringData()) {
                attributeValue = kiwiRecord.GetTuples(i).GetStringData();
            } else if (kiwiRecord.GetTuples(i).HasRawData()) {
                attributeValue = kiwiRecord.GetTuples(i).GetRawData();
            }

            if (attributeName == "URL" && !!attributeValue) {
                url = attributeValue;
            }

            if (attributeName == "HTTPResponse" && !!attributeValue) {
                originalDoc = attributeValue;
            }
        }

        if (!!originalDoc && !!url && url.size() < MAX_PATH_LENGTH) {
            TString result = UnpackOriginalDoc(url, originalDoc);
            TString host;
            TString path;
            TVector<TString> parts;
            SplitUrl(url, &host, &path);
            StringSplitter(path).Split('/').SkipEmpty().Collect(&parts);
            Cerr << url << Endl;
            TString currentDir = rootDir;
            TString fileName;
            bool isGoodPath = true;

            for (size_t i = 0; i < parts.size(); ++i) {
                currentDir += "/" + parts[i];
                Cerr << parts[i] << Endl;

                if (i + 1 != parts.size()) {
                    TFsPath fsPath(currentDir);
                    if (fsPath.Exists() && fsPath.IsFile()) {
                        isGoodPath = false;
                        break;
                    }
                    else {
                        MakeDirIfNotExist(currentDir);
                    }
                }
                else {
                    fileName = parts[i];
                }
            }

            if (!isGoodPath) {
                continue;
            }

            if (!IsDir(currentDir)) {
                TOFStream outputFile(currentDir);
                outputFile << result << Endl;
                outputFile.Finish();
                ++filesNumber;
            }
        }
    }

    Cerr << "Files processed: " << filesNumber << Endl;
 }



int main(int argc, const char** argv) {
    try {
        TFormatHostDumpConfig config(argc, argv);
        FormatHostDump(config);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }

    return 0;
}
