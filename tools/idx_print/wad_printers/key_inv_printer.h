#pragma once

#include <kernel/doom/key/decoded_key.h>
#include <kernel/doom/key/key_decoder.h>

#include <tools/idx_print/utils/options.h>

template <class KeyIo, class DocIo>
class TKeyInvWadPrinter : public TBaseWadPrinter {
    using TDecoder = NDoom::TKeyDecoder;
    using TKeyReader = typename KeyIo::TReader;
    using TDocSearcher = typename DocIo::TSearcher;
public:
    using THit = typename TDocSearcher::THit;
    using TKey = typename TKeyReader::TKey;
    using TKeyRef = typename TKeyReader::TKeyRef;

    TKeyInvWadPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TBaseWadPrinter(options, wad)
        , KeyReader_(wad)
        , DocSearcher_(wad)
        , Decoder_(options.YandexEncoded ? NDoom::RecodeToUtf8DecodingOption : NDoom::NoDecodingOptions)
    {
        FillKeys();
    }

    static TLumpSet UsedGlobalLumps() {
        return {
            NDoom::TWadLumpId(DocIo::IndexType, NDoom::EWadLumpRole::HitsModel),
            NDoom::TWadLumpId(KeyIo::IndexType, NDoom::EWadLumpRole::KeysModel),
            NDoom::TWadLumpId(KeyIo::IndexType, NDoom::EWadLumpRole::Keys),
            NDoom::TWadLumpId(KeyIo::IndexType, NDoom::EWadLumpRole::KeyFat),
            NDoom::TWadLumpId(KeyIo::IndexType, NDoom::EWadLumpRole::KeyIdx)
        };
    }

    static TLumpSet UsedDocLumps() {
        return {
            NDoom::TWadLumpId(DocIo::IndexType, NDoom::EWadLumpRole::Hits),
            NDoom::TWadLumpId(DocIo::IndexType, NDoom::EWadLumpRole::HitSub)
        };
    }

    virtual const char* Name() override {
        return "FactorAnn";
    }

private:
    void FillKeys() {
        TKeyRef key;
        ui32 termIndex = 0;
        while (KeyReader_.ReadKey(&key, &termIndex)) {
            if (termIndex >= Keys_.size())
                Keys_.resize(termIndex + 1);
            NDoom::TDecodedKey decoded;
            if (Decoder_.Decode(key, &decoded))
                Keys_[termIndex] = decoded;
        }
    }

    virtual void DoPrint(ui32 docId, IOutputStream* out) override {
        typename TDocSearcher::TIterator iterator;

        if (!DocSearcher_.Find(docId, &iterator))
            return;

        ui32 currentKeyId = -1;
        THit hit;
        while (iterator.ReadHit(&hit)) {
            ui32 keyId = hit.DocId();
            NDoom::TDecodedKey key = Keys_[keyId];
            hit.SetDocId(docId);
            if (CheckKey(key.Lemma())) {
                if (currentKeyId != keyId) {
                    *out << key << "\n";
                    currentKeyId = keyId;
                }
                *out << "\t" << hit << "\n";
            }
        }
    }

    virtual void PrintKeys(IOutputStream* out) override {
        for (const auto& key: Keys_)
            if (CheckKey(key.Lemma()))
                *out << key << "\n";
    }

private:
    TVector<NDoom::TDecodedKey> Keys_;
    TKeyReader KeyReader_;
    TDocSearcher DocSearcher_;
    TDecoder Decoder_;
};
