#include "numserializer.h"
#include "numeratorevents.h"

///////////////////////////////////////////////////////////////////////////////

#pragma pack(push, 4)
struct TEntries {
    ui16 Version;
    ui16 Properties;
    ui32 Events;
    ui32 Chunks;
    ui32 End;
};
#pragma pack(pop)

static TStringBuf GetChunks(const TStringBuf& data) {
    const TEntries* entries = (const TEntries*)data.data();
    return TStringBuf(data.data() + entries->Chunks, entries->End - entries->Chunks);
}

static TStringBuf GetEvents(const TStringBuf& data) {
    const TEntries* entries = (const TEntries*)data.data();
    return TStringBuf(data.data() + entries->Events, entries->Chunks - entries->Events);
}

static TStringBuf GetProperties(const TStringBuf& data) {
    const TEntries* entries = (const TEntries*)data.data();
    return TStringBuf(data.data() + entries->Properties, entries->Events - entries->Properties);
}

///////////////////////////////////////////////////////////////////////////////

TNumeratorEvents::TNumeratorEvents(const TBuffer& data)
    : TNumeratorEvents(TStringBuf(data.Data(), data.Size()))
{
}

TNumeratorEvents::TNumeratorEvents(const TStringBuf& data)
    : DocProps(CreateParsedDocProperties())
{
    Y_VERIFY(data, "numerator events is null");

    Events = GetEvents(data);
    DocProps->Load(GetProperties(data));

    Storage.SetPeerMode(NHtml::TStorage::ExternalText);

    {
        TStringBuf chunks = GetChunks(data);
        NHtml::TParserResult parsed(Storage);
        if (!NHtml::NumerateHtmlChunks(NHtml::TChunksRef(chunks.data(), chunks.size()), &parsed)) {
            ythrow yexception() << "can't deserialize html chunks";
        }
    }
}

TNumeratorEvents::~TNumeratorEvents() {
}

void TNumeratorEvents::Numerate(INumeratorHandler& handler, const char* zoneData, size_t zoneDataSize) const {
    Deserialize(Events.data(), Events.size(), &handler, Storage, DocProps.Get(), zoneData, zoneDataSize);
}

const IParsedDocProperties* TNumeratorEvents::GetDocProperties() const {
    return DocProps.Get();
}

///////////////////////////////////////////////////////////////////////////////

TBuffer SerializeNumeratorEvents(const NHtml::TChunksRef& chunks, const TBuffer& events, TAutoPtr<IParsedDocProperties> props) {
    const TBuffer properties(props->Serialize());

    TEntries entries;
    entries.Version = 1;
    entries.Properties = sizeof(entries);
    entries.Events = entries.Properties + properties.Size();
    entries.Chunks = entries.Events + events.Size();
    entries.End = entries.Chunks + chunks.size();

    TBuffer tmp;
    tmp.Append((const char*)&entries, sizeof(entries));
    tmp.Append(properties.Data(), properties.Size());
    tmp.Append(events.Data(), events.Size());
    tmp.Append(chunks.data(), chunks.size());

    return tmp;
}

///////////////////////////////////////////////////////////////////////////////
