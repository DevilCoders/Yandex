#include  <cstdio>
#include  <cstdlib>

#include  <util/system/defaults.h>
#include  <util/generic/string.h>
#include  <util/generic/cast.h>
#include  <library/cpp/deprecated/fgood/fgood.h>

#include  "remap_reader.h"

bool TTrivialRemapReader::Remap(ui32 docId, ui32& result)
{
    result = docId;
    return true;
}

/****************************************** TRemapReader ******************************************/

TRemapReader::TRemapReader(const char* filename)
    : FIn(filename, RdOnly)
{
    Length = SafeIntegerCast<ui32>(FIn.GetLength() / sizeof(ui32));
}

TRemapReader::~TRemapReader()
{
}

bool TRemapReader::Remap(ui32 docId, ui32& result)
{
    if (docId >= Length) {
        result = (ui32)-1;
        return false;
    }

    FIn.Seek(sizeof(ui32)*docId, sSet);
    FIn.Read(&result, sizeof(result));
    return result != (ui32)-1;
}

ui32 TRemapReader::GetLength() const
{
    return Length;
}

/****************************************** TRemapReaderCached ******************************************/

TRemapReaderCached::TRemapReaderCached(const char* filename)
{
    TFile fIn(filename, RdOnly | OpenExisting);
    ui32 length = (ui32)(fIn.GetLength()/sizeof(ui32));
    RemapData.resize(length);
    fIn.Seek(0, sSet);
    fIn.Read(RemapData.data(), sizeof(ui32)*length);
}

bool TRemapReaderCached::Remap(ui32 docId, ui32& result)
{
    if (docId >= RemapData.size())
        result = (ui32)-1;
    else
        result = RemapData[docId];
    return result != (ui32)-1;
}

ui32 TRemapReaderCached::GetLength() const
{
    return (ui32)RemapData.size();
}

/****************************************** TInvRemapReader ******************************************/

TInvRemapReader::TInvRemapReader(const char* fileName)
{
    TFile fIn(fileName, RdOnly | OpenExisting);
    i64 length = fIn.GetLength();
    Length = (ui32)(length/sizeof(ui32));
    TVector<ui32> remap;
    {
        remap.resize(Length);
        fIn.Read(remap.data(), sizeof(ui32)*Length);
    }
    for (ui32 i = 0; i < Length; ++i)
        InvRemap.insert(TInvRemap::value_type(remap[i], i));
}

ui32 TInvRemapReader::GetLength() const
{
    return Length;
}

bool TInvRemapReader::Remap(ui32 docId, ui32& result)
{
    TInvRemap::const_iterator toDocId = InvRemap.find(docId);
    if (toDocId != InvRemap.end()) {
        result = toDocId->second;
        return true;
    } else {
        return false;
    }
}

ui32 GetRemap(const char* input, ui32 docId)
{
    TRemapReader reader(input);
    return reader.Get(docId);
}

ui32 GetInvRemap(const char* input, ui32 docId)
{
    TInvRemapReader reader(input);
    return reader.Get(docId);
}
