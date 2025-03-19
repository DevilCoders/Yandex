#pragma once

#include <library/cpp/json/json_reader.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>
#include <util/string/cast.h>

#include <numeric>

namespace NNeuralNetApplier {

template <class TIndexType = size_t, bool useSwap = false, class TVec1, class TVec2>
void SortIndices(TVec1& indices, TVec2& vals) {
    if (Y_UNLIKELY(indices.size() != vals.size())) {
        Cerr << "ERROR in SortIndices: +indices != +vals";
        Y_ASSERT(false);

        indices.clear();
        vals.clear();
        return;
    }

    Y_ENSURE(indices.size() <= Max<TIndexType>());
    TVector<TIndexType> p(indices.size());
    std::iota(p.begin(), p.end(), 0);
    Sort(p.begin(), p.end(),
        [&](auto i, auto j){ return indices[i] < indices[j]; });

    TVector<typename TVec1::value_type> indicesSorted(indices.size());
    TVector<typename TVec2::value_type> valsSorted(vals.size());

    for (size_t i = 0; i < p.size(); ++i) {
        indicesSorted[i] = indices[p[i]];
        valsSorted[i] = vals[p[i]];
    }
    if constexpr(useSwap) {
        indices.swap(indicesSorted);
        vals.swap(valsSorted);
    } else {
        indices.assign(indicesSorted.begin(), indicesSorted.end());
        vals.assign(valsSorted.begin(), valsSorted.end());
    }
}

// parses maps of arrays, like
// {"texts": ["hakau", "agsdf"], "times": ["1", "2"], "values": [2,3]}"
class TRepeatedFieldParserCallbacks : public NJson::TJsonCallbacks {
public:
    TVector<TString> Keys;
    TVector<TVector<TString>> Values;

    TRepeatedFieldParserCallbacks()
        : NJson::TJsonCallbacks(true)
        , CurrentState(START)
    {
    }

    void Reset() {
        Keys.clear();
        Values.clear();
        CurrentState = START;
    }

    bool OnNull() override {
        ythrow yexception() << "Null not allowed in a repetead field";
    }
    bool OnBoolean(bool val) override {
        SetValue(ToString(val));
        return true;
    }
    bool OnInteger(long long val) override {
        SetValue(ToString(val));
        return true;
    }
    bool OnUInteger(unsigned long long val) override {
        SetValue(ToString(val));
        return true;
    }
    bool OnString(const TStringBuf &val) override {
        SetValue(TString(val.data(), val.size()));
        return true;
    }
    bool OnDouble(double val) override {
        SetValue(ToString(val));
        return true;
    }
    bool OnOpenArray() override {
        OpenComplexValue(NJson::JSON_ARRAY);
        return true;
    }
    bool OnCloseArray() override {
        CloseComplexValue();
        return true;
    }
    bool OnOpenMap() override {
        OpenComplexValue(NJson::JSON_MAP);
        return true;
    }
    bool OnCloseMap() override {
        CloseComplexValue();
        return true;
    }
    bool OnMapKey(const TStringBuf &val) override {
        if (CurrentState == IN_MAP) {
            Keys.push_back(TString(val.data(), val.size()));
            Values.emplace_back();
            CurrentState = AFTER_MAP_KEY;
            return true;
        }
        ythrow yexception() << "Writing map key while not in map keys state";
    }
private:
    enum {
        START,
        AFTER_MAP_KEY,
        IN_MAP,
        IN_ARRAY,
        FINISH
    } CurrentState;

    void SetValue(const TString& value) {
        switch (CurrentState) {
            case IN_ARRAY:
                Values.back().push_back(value);
                break;
            case AFTER_MAP_KEY:
                CurrentState = IN_MAP;
                return;
            case START:
                ythrow yexception() << "Json should be a map!";
            case IN_MAP:
            case FINISH:
                ythrow yexception() << "Incorrect json!";
        }
    }

    void OpenComplexValue(NJson::EJsonValueType type) {
        switch (CurrentState) {
            case START:
                if (type != NJson::JSON_MAP) {
                    ythrow yexception() << "Json should be a map!";
                }
                CurrentState = IN_MAP;
                break;
            case IN_ARRAY:
                ythrow yexception() << "Map value should be an array of strings!";
                break;
            case AFTER_MAP_KEY:
                if (type != NJson::JSON_ARRAY) {
                    ythrow yexception() << "Map value should be an array of strings!";
                }
                CurrentState = IN_ARRAY;
                break;
            default:
                ythrow yexception() << "Unsupported json format!";
        }
    }
    void CloseComplexValue() {
        if (CurrentState == IN_ARRAY) {
            CurrentState = IN_MAP;
        } else if (CurrentState == IN_MAP) {
            CurrentState = FINISH;
        } else {
            Y_VERIFY(false);
        }
    }
};

};
