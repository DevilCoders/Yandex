#include "reader.h"

#include <library/cpp/reverse_geocoder/library/log.h>

#include <util/generic/vector.h>
#include <util/network/address.h>

using namespace NReverseGeocoder;
using namespace NOpenStreetMap;

static const size_t MAX_BLOB_HEADER_SIZE = 64 * 1024;
static const size_t MAX_UNCOMPRESSED_BLOB_SIZE = 32 * 1024 * 1024;

bool NReverseGeocoder::NOpenStreetMap::TReader::Read(NProto::TBlobHeader* header, NProto::TBlob* blob) {
    TVector<char> rawHeader;
    rawHeader.reserve(MAX_BLOB_HEADER_SIZE);

    TVector<char> rawBlob;
    rawBlob.reserve(MAX_UNCOMPRESSED_BLOB_SIZE);

    TGuard<TMutex> lock(Mutex_);

    i32 headerSize = 0;
    if (InputStream_->Load((char*)&headerSize, sizeof(headerSize)) < sizeof(headerSize))
        return false;

    headerSize = ntohl(headerSize);

    rawHeader.resize(headerSize);
    if (InputStream_->Load(rawHeader.data(), rawHeader.size()) != rawHeader.size()) {
        LogError("Unable read header!");
        return false;
    }

    if (!header->ParseFromArray(rawHeader.data(), rawHeader.size())) {
        LogError("Unable parse header!");
        return false;
    }

    const i32 blobSize = header->GetDataSize();

    rawBlob.resize(blobSize);
    if (InputStream_->Load(rawBlob.data(), rawBlob.size()) != rawBlob.size()) {
        LogError("Unable read blob!");
        return false;
    }

    if (!blob->ParseFromArray(rawBlob.data(), rawBlob.size())) {
        LogError("Unable parse blob!");
        return false;
    }

    return true;
}
