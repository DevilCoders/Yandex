#include <util/stream/file.h>
#include <util/ysaveload.h>
#include <library/cpp/on_disk/chunks/chunked_helpers.h>

#include "url_to_positions.h"

TUrl2PositionsReader::TUrl2PositionsReader(const TString& filename)
    : Data(TBlob::FromFile(filename))
    , Url2Offset(GetBlock(Data, 0))
    , Positions(GetBlock(Data, 1))
    , Size(GetBlock(Data, 2))
{
}

size_t TUrl2PositionsReader::GetNUrls() const {
    return Size.Get();
}

bool TUrl2PositionsReader::Get(const TString& url, TPositions* result) const {
    ui64 offset;
    if (Url2Offset.Get(url.data(), &offset)) {
        ui32 count = Positions.At((size_t)offset);
        result->resize(count);
        ui64 now = offset + 1;
        for (ui32 i = 0; i < count; ++i)
            (*result)[i] = Positions.At((size_t)now++);
        return true;
    } else {
        return false;
    }
}

void MakeUrl2Positions(const TUrls& urls, TUrl2Positions* url2Positions) {
    url2Positions->clear();
    for (size_t i = 0; i < urls.size(); ++i)
        (*url2Positions)[urls[i]].push_back((ui32)i);
}

void WriteUrl2Positions(size_t nUrls, const TUrl2Positions& url2positions, const TString& filename) {
    TTrieMapWriter<ui64> url2offset;
    TYVectorWriter<ui32> positions;
    for (TUrl2Positions::const_iterator toUrl = url2positions.begin(); toUrl != url2positions.end(); ++toUrl) {
        url2offset.Add(toUrl->first.data(), positions.Size());
        positions.PushBack((ui32)toUrl->second.size());
        for (TPositions::const_iterator toPos = toUrl->second.begin(); toPos != toUrl->second.end(); ++toPos)
            positions.PushBack(*toPos);
    }
    TFixedBufferFileOutput fOut(filename);
    TChunkedDataWriter writer(fOut);
    WriteBlock(writer, url2offset);
    WriteBlock(writer, positions);
    TSingleValueWriter<ui32> size;
    size.Set((ui32)nUrls);
    WriteBlock(writer, size);
    writer.WriteFooter();
}
