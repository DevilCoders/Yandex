#pragma once

#include <library/cpp/html/face/event.h>
#include <library/cpp/html/storage/storage.h>
#include <library/cpp/html/face/blob/chunkslist.h>

class IParsedDocProperties;
class INumeratorHandler;

class TNumeratorEvents: private TNonCopyable, public TSimpleRefCount<TNumeratorEvents> {
public:
    explicit TNumeratorEvents(const TStringBuf& data);
    explicit TNumeratorEvents(const TBuffer& data);
    TNumeratorEvents(const char*) = delete;
    ~TNumeratorEvents();

    // Replay numerator events.
    void Numerate(INumeratorHandler& handler, const char* zoneData = nullptr, size_t zoneDataSize = 0) const;

    const IParsedDocProperties* GetDocProperties() const;

private:
    TStringBuf Events;
    THolder<IParsedDocProperties> DocProps;
    NHtml::TStorage Storage;
};

TBuffer SerializeNumeratorEvents(const NHtml::TChunksRef& chunks, const TBuffer& events, TAutoPtr<IParsedDocProperties> props);
