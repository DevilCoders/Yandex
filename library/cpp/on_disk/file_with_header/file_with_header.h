#pragma once
#include <util/folder/path.h>
#include <util/memory/blob.h>
#include <util/datetime/base.h>
#include <util/system/src_location.h>

namespace NFileWithHeader {

struct TSymVer {
    size_t MajorVer = 0;
    size_t MinorVer = 0;
    size_t InfoVer = 0;

    constexpr TSymVer() = default;
    constexpr TSymVer(size_t major, size_t minor, size_t info)
        : MajorVer(major)
        , MinorVer(minor)
        , InfoVer(info)
    {}

    TString ToString() const;
    static TSymVer FromString(TStringBuf);

    constexpr static bool CheckCompability(const TSymVer fileVersion, const TSymVer readerVersion) {
        return fileVersion.MajorVer == readerVersion.MajorVer
            && readerVersion.MinorVer >= fileVersion.MinorVer;
    }
};

struct THeaderInfo {
    TString CommonName;
    TString CustomHumanReadableComment;
    TSymVer FormatVersion;

    TSourceLocation CppBuilderLocation = __LOCATION__;
    TString CustomHeaderData;

    void DefaultCompabilityCheck(TStringBuf expectedCommonName, TSymVer readerVersion) const {
        Y_ENSURE(expectedCommonName == CommonName &&
            TSymVer::CheckCompability(FormatVersion, readerVersion),
            "Versions mismatch: can't read file with version " << CommonName << "-" << FormatVersion.ToString()
             << " via code with version " << expectedCommonName << "-" << readerVersion.ToString()
        );
    }
};

struct TFullHeaderInfo : public THeaderInfo {
    TSymVer FileWithHeaderLibFormatInfo;
    TInstant BuildTimeInfo;
    TString BuildSvnInfo;
};

class TFileWithHeaderBuilder : public IOutputStream {
    class TImpl;
    THolder<TImpl> Impl;
public:
    TFileWithHeaderBuilder(
        const THeaderInfo& header,
        IOutputStream& dst);

    ~TFileWithHeaderBuilder() override;
    void DoWrite(const void* buf, size_t len) override;
};

class TFileWithHeaderReader {
    class TImpl;
    THolder<TImpl> Impl;
public:
    TFileWithHeaderReader(const TFsPath&);
    TFileWithHeaderReader(TBlob);
    ~TFileWithHeaderReader();

    TFileWithHeaderReader(TFileWithHeaderReader&& other);
    TFileWithHeaderReader& operator=(TFileWithHeaderReader&& other);

    const TFullHeaderInfo& GetHeader() const;

    TBlob GetBody() const;
    THolder<IZeroCopyInput> GetBodyStream() const;
};

}

