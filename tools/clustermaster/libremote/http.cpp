#include "http.h"

#include <library/cpp/string_utils/base64/base64.h>

#include <util/memory/blob.h>
#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/string/util.h>
#include <util/system/file.h>
#include <util/system/fs.h>
#include <util/system/tempfile.h>

namespace fetch {

void SslStaticInit() {
    static const unsigned char ArchiveData[] = {
        #include "archive.inc"
    };

    static const TArchiveReader archive(TBlob::NoCopy(ArchiveData, sizeof(ArchiveData)));

    const TAutoPtr<IInputStream> certArchive = archive.ObjectByKey("/cacert.pem");

    TFile certFile(MakeTempName(), CreateAlways | RdWr);

    try {
        TUnbufferedFileOutput certFileOutput(certFile);
        TransferData(certArchive.Get(), &certFileOutput);
        TSslSocketBase::StaticInit(certFile.GetName().data());
    } catch (...) {
        certFile.Close();
        NFs::Remove(certFile.GetName());
        throw;
    }

    certFile.Close();
    NFs::Remove(certFile.GetName());
}

TUrlFetcher::TUrlFetcher(const TStringBuf& stringUrl) {
    const NUri::TParseFlags parseFlags = 0
        | NUri::TFeature::FeatureSchemeKnown
        | NUri::TFeature::FeatureCheckHost
        | NUri::TFeature::FeatureRemoteOnly
        | NUri::TFeature::FeatureNoRelPath;

    THttpURL url;
    THttpURL::TParsedState parsedUrl = url.Parse(stringUrl, parseFlags);

    if (parsedUrl != THttpURL::ParsedOK || !url.Get(THttpURL::FieldHost))
        throw yexception() << "Can't parse url " << stringUrl;

    TBase::SetHost(url.GetHost().data(), url.GetPort(), url.GetScheme());

    url.Print(PathQuery, NUri::TField::FlagPath | NUri::TField::FlagQuery);
}

int TUrlFetcher::Fetch(IOutputStream& out, const TVector<TString>& headers) {
    THttpHeader recvdHeader;

    TVector<const char*> sentHeaders(headers.size() + 1);
    ui32 index = 0;

    for (const TString& header : headers) {
        sentHeaders[index] = header.c_str();
        ++index;
    }

    Y_VERIFY(index == headers.size());
    sentHeaders[headers.size()] = nullptr;

    Buffer.Clear();

    TBase::Fetch(&recvdHeader, PathQuery.data(), sentHeaders.data(), 0);
    TBase::Disconnect();

    TCompressedHttpReader<TMemoReader> httpReader;
    httpReader.TMemoReader::Init(Buffer.Data(), Buffer.Size());

    if (httpReader.Init(&recvdHeader, 1))
        return 0;

    void* data = nullptr;
    size_t size = 0;

    while (size = httpReader.Read(data))
        out.Write(data, size);

    return recvdHeader.http_status;
}

TString TUrlFetcher::BasicAuthHeaderLine(const TString& user, const TString& password) {
    return TString("Authorization: Basic ").append(Base64Encode(TString(user).append(':').append(password))).append("\r\n");
}

TString TUrlFetcher::OAuthHeaderLine(const TString& token) {
    return TString("Authorization: OAuth ").append(token).append("\r\n");
}

} // namespace fetch
