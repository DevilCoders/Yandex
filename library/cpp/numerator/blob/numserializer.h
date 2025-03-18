#pragma once

#include <library/cpp/html/face/event.h>
#include <library/cpp/html/face/zoneconf.h>
#include <library/cpp/html/storage/storage.h>
#include <library/cpp/numerator/numerate.h>

class THtConfigurator;

class TNumerSerializer: public INumeratorHandler {
public:
    // number of zone buffers must be equal to number of configurators
    // data in outBuffer does not depend on configuration because zone data is stored
    // to zoneBuffers and types of spaces (ST_ZONEOPN, ST_ZONECLS) are corrected during deserialization
    TNumerSerializer(TBuffer& outBuffer, IParsedDocProperties* props,
                     const THtConfigurator* configs, size_t configCount, TBuffer* zoneBuffers);
    ~TNumerSerializer() override;

    void OnTextStart(const IParsedDocProperties* parser) override;
    void OnTextEnd(const IParsedDocProperties* parser, const TNumerStat& stat) override;
    void OnParagraphEnd() override;
    void OnAddEvent(const THtmlChunk& e) override;
    void OnRemoveEvent(const THtmlChunk& e) override;
    void OnTokenBegin(const THtmlChunk& e, unsigned offset) override;
    void OnTokenEnd(const THtmlChunk& e, unsigned offset) override;
    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* zone, const TNumerStat& stat) override;

    void OnTokenStart(const TWideToken& tok, const TNumerStat&) override;
    void OnSpaces(TBreakType type, const wchar16* token, unsigned len, const TNumerStat& stat) override;

private:
    void AppendToBuffer(ui8 type, const THtmlChunk* chunk);

    void StoreChunkAndStat(ui8 type, const THtmlChunk& chunk, const TNumerStat& stat);

private:
    class TStatChecker;
    class TZoneExtractor;

    TBuffer* const Out_;
    TBuffer* const ZoneBuffers_;
    const THolder<TStatChecker> StatChecker_;

    TNlpInputDecoder Decoder_;
    TVector<TAutoPtr<TZoneExtractor>> Extractors_;
    int BaseIdx_;
    int LastIdx_;
};

void Deserialize(const void* data, size_t size, INumeratorHandler* handler, const NHtml::TStorage& storage,
                 const IParsedDocProperties* parser = nullptr, const char* zoneData = nullptr, size_t zoneDataSize = 0);
