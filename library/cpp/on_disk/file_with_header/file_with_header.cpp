#include "file_with_header.h"

#include <library/cpp/svnversion/svnversion.h>
#include <util/string/split.h>
#include <util/string/escape.h>
#include <util/generic/bt_exception.h>
#include <util/generic/hash.h>
#include <util/stream/mem.h>


namespace NFileWithHeader {
static constexpr TSymVer HeaderFormatVer = TSymVer{2, 0, 0};
static constexpr size_t AlignmentRequirement = 32;

class TFileWithHeaderReadException : public TWithBackTrace<yexception> {};

class THeaderIO {
public:
    static constexpr size_t MinimalNumberOfHeadLines = 9;
    static constexpr size_t EmptyLinesOnEndNum = 3;

    static TFullHeaderInfo ExtendHeaderInfo(const THeaderInfo& src) {
        TFullHeaderInfo res;
        (THeaderInfo&) res = src;
        res.BuildTimeInfo = Now();
        res.FileWithHeaderLibFormatInfo = HeaderFormatVer;
        res.BuildSvnInfo = GetProgramSvnVersion();

        return res;
    }

    static void WriteHeader(IOutputStream& realOut, const TFullHeaderInfo& h) {
        TStringStream out;
        size_t lines = 0;
        out << EscapeC(h.CommonName) << "\t" << h.FormatVersion.ToString() << "\n";
        lines += 1;
        if (h.CustomHumanReadableComment) {
            out << "Comment\t" << EscapeC(h.CustomHumanReadableComment) << "\n";
            lines += 1;
        }
        out << "HeaderVer\t" << h.FileWithHeaderLibFormatInfo.ToString() << "\n";
        lines += 1;
        out << "BuildTime\t" << h.BuildTimeInfo.ToString() << "\n";
        lines += 1;
        out << "BuildInfo\t" << EscapeC(h.BuildSvnInfo) << "\n";
        lines += 1;
        out << "CppBuilderLocation\t" << TString::Join(
            h.CppBuilderLocation.File, ":", ToString(h.CppBuilderLocation.Line)) << "\n";
        lines += 1;

        if (h.CustomHeaderData) {
            out << "CustomHeader\t" << EscapeC(h.CustomHeaderData);
            out << "\n";
            lines += 1;
        }

        for (size_t i = 0; i < EmptyLinesOnEndNum || lines < MinimalNumberOfHeadLines; ++i) {
            out << "\n";
            lines += 1;
        }

        out << "LastLine, perform alignmentskip\t";
        TString str = out.Str();
        realOut << str;
        size_t writtenSize = str.size() + 1; //+1 for last \n

        while (writtenSize % AlignmentRequirement != 0) {
            writtenSize += 1;
            realOut << '-';
        }
        realOut << "\n";
    }

    //NOTE: move ptr of input-stream to the position right before body-part; will return result position
    static TFullHeaderInfo ReadHeader(TStringBuf& in) {
        const char* inputStart = in.begin();

        TFullHeaderInfo res;
        TStringBuf alignRequirementSkipLine;
        try {
            size_t lines = 0;

            {
                TStringBuf curLine = in.NextTok('\n');
                lines += 1;

                TStringBuf symver;
                TStringBuf common;
                Split(curLine, '\t', common, symver);
                res.CommonName = UnescapeC(common);
                res.FormatVersion = TSymVer::FromString(symver);
            }

            THashMap<TStringBuf, TStringBuf> readData;
            size_t emptyLinesInRow = 0;
            while (emptyLinesInRow < EmptyLinesOnEndNum || lines < MinimalNumberOfHeadLines) {
                Y_ENSURE(in.size() > 0);
                TStringBuf curLine = in.NextTok('\n');
                lines += 1;

                if (!curLine) {
                    emptyLinesInRow += 1;
                    continue;
                } else {
                    emptyLinesInRow = 0;
                }

                TStringBuf a, b;
                Split(curLine, '\t', a, b);
                readData[a] = b;
            }
            alignRequirementSkipLine = in.NextTok('\n');
            Y_ENSURE(alignRequirementSkipLine.NextTok("\t") == "LastLine, perform alignmentskip");
            Y_ENSURE((in.begin() - inputStart) % AlignmentRequirement == 0);

            if (auto ptr = readData.FindPtr("HeaderVer")) {
                res.FileWithHeaderLibFormatInfo = TSymVer::FromString(*ptr);
                Y_ENSURE(TSymVer::CheckCompability(res.FileWithHeaderLibFormatInfo, HeaderFormatVer));
            }

            if (auto ptr = readData.FindPtr("Comment")) {
                res.CustomHumanReadableComment = UnescapeC(*ptr);
            }

            if (auto ptr = readData.FindPtr("BuildInfo")) {
                res.BuildSvnInfo = UnescapeC(*ptr);
            }

            if (auto ptr = readData.FindPtr("CustomHeader")) {
                res.CustomHeaderData = UnescapeC(*ptr);
            }

            if (auto ptr = readData.FindPtr("BuildTime")) {
                res.BuildTimeInfo = TInstant::ParseIso8601Deprecated(*ptr);
            }

            if (auto ptr = readData.FindPtr("CppBuilderLocation")) {
                TStringBuf file;
                TStringBuf line = 0;
                ptr->RSplit(":", file, line);
                res.CppBuilderLocation = {file, FromString<int>(line)};
            }
        } catch (yexception& e) {
            ythrow TFileWithHeaderReadException{} << "failed to parse header: "
                << e.what() << "\n\n!FailedToReadPart!:\n"
                << TStringBuf(inputStart, in.begin()).Trunc(200) << "...";
        }
        return res;
    }
};

static constexpr char SymVerDivisionSymbol = '.';

TString TSymVer::ToString() const {
    return TString::Join(
        ::ToString(MajorVer), SymVerDivisionSymbol, ::ToString(MinorVer), SymVerDivisionSymbol, ::ToString(InfoVer)
    );
}

NFileWithHeader::TSymVer NFileWithHeader::TSymVer::FromString(TStringBuf in) {
    try {
        TSymVer res;
        Split(in, SymVerDivisionSymbol, res.MajorVer, res.MinorVer, res.InfoVer);
        return res;
    } catch (yexception& e) {
        ythrow TFileWithHeaderReadException{} <<
            "failed to parse sym-ver string '" << in.SubStr(0, 10)
            << (in.size() > 10 ? "<truncated>" : "")
            << "'\n"
            << e.what();
    }
}

class TFileWithHeaderBuilder::TImpl {
public:
    IOutputStream& Dst;
    TImpl(IOutputStream& dst)
        : Dst(dst)
    {}

    void WriteHeader(const THeaderInfo& header) {
        TStringStream tmp;
        //THeaderIO::WriteHeader(tmp, THeaderIO::ExtendHeaderInfo(header));
        //Cerr << "at write skip size " << tmp.Size() << Endl;
        THeaderIO::WriteHeader(Dst, THeaderIO::ExtendHeaderInfo(header));
    }
};

TFileWithHeaderBuilder::TFileWithHeaderBuilder(const THeaderInfo & header, IOutputStream& dst) {
    Impl = MakeHolder<TImpl>(dst);
    Impl->WriteHeader(header);
}

TFileWithHeaderBuilder::~TFileWithHeaderBuilder() {
    Impl->Dst.Flush();
}

void TFileWithHeaderBuilder::DoWrite(const void * buf, size_t len) {
    Impl->Dst.Write(buf, len);
}

class TFileWithHeaderReader::TImpl {
public:
    TBlob FullDataView;
    TBlob BodyDataView;
    TFullHeaderInfo HeaderInfo;

    TImpl(TBlob b) :
        FullDataView(b)
    {
        TStringBuf in(b.AsCharPtr(), b.Size());
        HeaderInfo = THeaderIO::ReadHeader(in);

        BodyDataView = FullDataView.SubBlob(in.begin() - b.AsCharPtr(), b.Size());
    }

};

TFileWithHeaderReader::TFileWithHeaderReader(const TFsPath& file) :
    TFileWithHeaderReader(TBlob::FromFile(file))
{}

TFileWithHeaderReader::TFileWithHeaderReader(TBlob b) {
    Impl = MakeHolder<TImpl>(b);
}

TFileWithHeaderReader::~TFileWithHeaderReader() = default;

TFileWithHeaderReader::TFileWithHeaderReader(TFileWithHeaderReader&& other) = default;

TFileWithHeaderReader& TFileWithHeaderReader::operator=(TFileWithHeaderReader&& other) = default;

const TFullHeaderInfo& TFileWithHeaderReader::GetHeader() const {
    return Impl->HeaderInfo;
}

TBlob TFileWithHeaderReader::GetBody() const {
    return Impl->BodyDataView;
}

THolder<IZeroCopyInput> TFileWithHeaderReader::GetBodyStream() const {
    return MakeHolder<TMemoryInput>(
        Impl->BodyDataView.AsCharPtr(), Impl->BodyDataView.Size());
}

}
