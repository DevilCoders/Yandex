#pragma once

#include <library/cpp/offroad/flat/flat_searcher.h>

#include <ysite/yandex/erf/flat_hash_table.h>

class TInvHashWadPrinter: public TBaseWadPrinter {
public:
    TInvHashWadPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TBaseWadPrinter(options, wad)
    {
        Init(options);
    }


    static const TLumpSet& UsedGlobalLumps()  {
        static TLumpSet lumps = {
            NDoom::TWadLumpId(NDoom::InvUrlHashesIndexType, NDoom::EWadLumpRole::Hits)
        };
        return lumps;
    }

    static const TLumpSet& UsedDocLumps() {
        static TLumpSet lumps = {
        };
        return lumps;
    }

    ui32 GetSize() const {
        return Data.size();
    }
private:
    TVector<std::pair<ui32, ui64>> Data;

    void PrintKeys(IOutputStream *out) override {
        for (ui32 i = 0; i < Data.size(); ++i) {
            *out << Data[i].first << " " << Data[i].second << Endl;
        }
    }

    void DoPrint(ui32 docId, IOutputStream* out) override {
        *out << Data[docId].first << " " << Data[docId].second << Endl;
    }

    void Init(const TIdxPrintOptions& options) {
        THolder<NDoom::IWad> wadReader = NDoom::IWad::Open(options.IndexPath);
        NOffroad::TFlatSearcher<ui32, ui64, NOffroad::TUi32Vectorizer, NOffroad::TUi64Vectorizer> flatSearcher
                                (wadReader->LoadGlobalLump(NDoom::TWadLumpId(NDoom::EWadIndexType::InvUrlHashesIndexType, NDoom::EWadLumpRole::Hits)));
        for (ui32 i = 0; i < flatSearcher.Size(); ++i) {
            ui32 curKey = flatSearcher.ReadKey(i);
            if (curKey != TFlatHashTable::EmptyDocId) {
                if (curKey >= Data.size()) {
                    Data.resize(curKey + 1, { 0, 0 });
                }
                Y_ENSURE(Data[curKey].first == 0, "DocIds are not unique");
                Data[curKey] = { curKey, flatSearcher.ReadData(i) };
            }
        }
    }

    const char* Name() override {
        return "InvHashWad";
    }
};
