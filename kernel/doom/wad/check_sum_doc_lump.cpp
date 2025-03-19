#include <library/cpp/offroad/streams/bit_input.h>

#include "check_sum_doc_lump.h"


TMaybe<NDoom::TCrcExtendCalcer::TCheckSum> NDoom::CalcBlobCheckSum(const TBlob& documentBlob, TConstArrayRef<char> checkSumRegion) {
    const char* documentBlobData = static_cast<const char*>(documentBlob.Data());
    const size_t documentBlobSize = documentBlob.Size();
    const char* checkSumRegionData = checkSumRegion.data();
    const size_t checkSumRegionSize = checkSumRegion.size();

    if (checkSumRegionSize > 0 && checkSumRegionSize != sizeof(NDoom::TCrcExtendCalcer::TCheckSum)) {
        return Nothing();
    }

    size_t regionBeforCheckSumSize = documentBlobSize;
    size_t regionAfterCheckSumSize = 0;
    if (checkSumRegionSize > 0) {
        regionBeforCheckSumSize = checkSumRegionData - documentBlobData;
        regionAfterCheckSumSize = (documentBlobData + documentBlobSize) - (checkSumRegionData + checkSumRegionSize);
    }

    TCrcExtendCalcer checkSumCalcer;
    NDoom::TCrcExtendCalcer::TCheckSum checkSum = checkSumCalcer(0, documentBlobData, regionBeforCheckSumSize);
    if (checkSumRegionSize) {
        NDoom::TCrcExtendCalcer::TCheckSum zeroCheckSum = 0;
        checkSum = checkSumCalcer(checkSum, static_cast<void*>(&zeroCheckSum), checkSumRegionSize);
    }
    checkSum = checkSumCalcer(checkSum, checkSumRegionData + checkSumRegionSize, regionAfterCheckSumSize);

    return checkSum;
}

TMaybe<NDoom::TCrcExtendCalcer::TCheckSum> NDoom::FetchCheckSum(TConstArrayRef<char> checkSumRegion) {
    if (checkSumRegion.size() != sizeof(TCrcExtendCalcer::TCheckSum)) {
        return Nothing();
    }

    ui64 checkSumData = 0;
    NOffroad::TBitInput reader{checkSumRegion};
    reader.Read(&checkSumData, 8 * sizeof(TCrcExtendCalcer::TCheckSum));
    return static_cast<TCrcExtendCalcer::TCheckSum>(checkSumData);
}
