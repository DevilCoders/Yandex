#include "file_with_header.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/random/easy.h>
#include <util/generic/xrange.h>

using namespace NFileWithHeader;

static TString RandomStr(size_t len) {
    TStringStream stream;
    for (auto i : xrange(len)) {
        Y_UNUSED(i);
        unsigned char c = Random();
        stream << c;
    }
    return stream.Str();
}

Y_UNIT_TEST_SUITE(FileWithHeader) {
    Y_UNIT_TEST(SimpleReadWrite) {
        TString file;
        auto location = __LOCATION__;
        {
            TStringStream stream;
            //creating file
            {
                THeaderInfo header;
                header.CommonName = "unittest";
                header.CustomHumanReadableComment = "test it";
                header.FormatVersion = {1, 2, 3};
                header.CppBuilderLocation = location;
                header.CustomHeaderData = "custom data";

                TFileWithHeaderBuilder builder(header, stream);
                builder << "body data";
            }
            file = stream.Str();
        }
        //Cerr << file << Endl;
        {
            //checking file
            TFileWithHeaderReader reader(TBlob::FromString(file));
            UNIT_ASSERT_NO_DIFF(TStringBuf(reader.GetBody().AsCharPtr(), reader.GetBody().Size()), "body data");
            UNIT_ASSERT_NO_DIFF(reader.GetHeader().CommonName, "unittest");
            UNIT_ASSERT_NO_DIFF(reader.GetHeader().CustomHumanReadableComment, "test it");
            UNIT_ASSERT(reader.GetHeader().FormatVersion.ToString() == TSymVer(1, 2, 3).ToString());
            UNIT_ASSERT_NO_DIFF(reader.GetHeader().CppBuilderLocation.File, location.File);
            UNIT_ASSERT(reader.GetHeader().CppBuilderLocation.Line == location.Line);
            UNIT_ASSERT_NO_DIFF(reader.GetHeader().CustomHeaderData, "custom data");
            UNIT_ASSERT(!reader.GetHeader().BuildSvnInfo.empty());
            UNIT_ASSERT(Now() - reader.GetHeader().BuildTimeInfo < TDuration::Minutes(5));
        }
    }

    Y_UNIT_TEST(SimpleFuzzing) {
        TString file;
        TString bodyCopy;

        THeaderInfo header;
        header.CommonName = RandomStr(10);
        header.CppBuilderLocation.File = "123";
        header.CppBuilderLocation.Line = (unsigned int)Random();
        header.CustomHeaderData = RandomStr(10);
        header.CustomHumanReadableComment = RandomStr(10);
        header.FormatVersion = TSymVer(Random(), Random(), Random());

        {
            TStringStream stream;
            TStringStream bodyStream;
            //creating file
            {
                TFileWithHeaderBuilder builder(header, stream);

                for (auto i : xrange(10000)) {
                    Y_UNUSED(i);
                    unsigned char c = Random();
                    builder << c;
                    bodyStream << c;
                }
            }
            file = stream.Str();
            bodyCopy = bodyStream.Str();
        }

        TFileWithHeaderReader reader(TBlob::FromString(file));
        UNIT_ASSERT(TStringBuf(reader.GetBody().AsCharPtr(), reader.GetBody().Size()) == bodyCopy);
        UNIT_ASSERT_NO_DIFF(reader.GetHeader().CommonName, header.CommonName);
        UNIT_ASSERT_NO_DIFF(reader.GetHeader().CustomHumanReadableComment, header.CustomHumanReadableComment);
        UNIT_ASSERT(reader.GetHeader().FormatVersion.ToString() == header.FormatVersion.ToString());
        UNIT_ASSERT(reader.GetHeader().CppBuilderLocation.File == header.CppBuilderLocation.File);
        UNIT_ASSERT(reader.GetHeader().CppBuilderLocation.Line == header.CppBuilderLocation.Line);
        UNIT_ASSERT_NO_DIFF(reader.GetHeader().CustomHeaderData, header.CustomHeaderData);
    }
}
