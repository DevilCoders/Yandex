#pragma once

#include "util.h"

#include <kernel/dssm_applier/nn_applier/lib/protos/repeated.pb.h>

#include <util/generic/cast.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <util/string/split.h>

namespace NNeuralNetApplier {

class TJsonParser {
public:
    TJsonParser() {
        ReadConfig.DontValidateUtf8 = true;
    }

    void ParseRecord(const TString& str) {
        Callbacks.Reset();
        TMemoryInput s(str.data(), str.size());
        ReadJson(&s, &ReadConfig, &Callbacks);
    }

    size_t GetKeysCount() const {
        return Callbacks.Keys.size();
    }

    const TString& GetKeyAt(size_t position) const {
        return Callbacks.Keys.at(position);
    }

    size_t GetValuesCount(size_t position) const {
        return Callbacks.Values.at(position).size();
    }

    const TString& GetValueAt(size_t position, size_t index) const {
        return Callbacks.Values.at(position).at(index);
    }

    static TStringBuf GetFormatName() {
        // Left empty for backwards compatibility
        return TStringBuf("");
    }
private:
    TRepeatedFieldParserCallbacks Callbacks;
    NJson::TJsonReaderConfig ReadConfig;
};

class TBinaryParser {
public:
    void ParseRecord(const TString& str) {
        if (!Proto.ParseFromString(str)) {
            ythrow yexception() << "Failed to parse TRepeatedFieldProto from " << str.Quote();
        }
    }

    size_t GetKeysCount() const {
        return Proto.EntriesSize();
    }

    const TString& GetKeyAt(size_t position) const {
        return Proto.GetEntries(position).GetKey();
    }

    size_t GetValuesCount(size_t position) const {
        return Proto.GetEntries(position).ValuesSize();
    }

    const TString& GetValueAt(size_t position, size_t index) const {
        return Proto.GetEntries(position).GetValues(index);
    }

    static TStringBuf GetFormatName() {
        return TStringBuf("Binary");
    }
private:
    TRepeatedFieldProto Proto;
};

}
