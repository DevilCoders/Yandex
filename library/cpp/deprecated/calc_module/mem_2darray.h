#pragma once

#include "simple_module.h"

template <class T, bool useCMapFormat = false>
class TMem2DArray: public TSimpleModule {
private:
    typedef TVector<T> TRow;
    TVector<TRow> Array;

    TMem2DArray()
        : TSimpleModule("TMem2DArray")
    {
        Bind(this).template To<size_t, &TMem2DArray::GetSize>("size_output");
        Bind(this).template To<size_t, size_t&, &TMem2DArray::GetLength>("length_output");
        Bind(this).template To<size_t, size_t, const T*&, &TMem2DArray::GetData>("data_output");
        Bind(this).template To<&TMem2DArray::NewLine>("start,new_line");
        Bind(this).template To<const T&, &TMem2DArray::Write>("input");
        Bind(this).template To<&TMem2DArray::Finish>("finish");
    }

public:
    static TCalcModuleHolder BuildModule() {
        return new TMem2DArray();
    }

private:
    void Finish() {
        Array.clear();
    }
    void NewLine() {
        Array.push_back(TRow());
    }
    void Write(const T& val) {
        Y_ASSERT(Array.size());
        Array.back().push_back(val);
    }
    size_t GetSize() {
        return Array.size();
    }
    void GetLength(size_t pos, size_t& length) {
        Y_ASSERT(Array.size() > pos);
        length = Array[pos].size();
    }
    void GetData(size_t i, size_t j, const T*& res) {
        if (useCMapFormat) {
            --j;
        }
        Y_ASSERT(Array.size() > i);
        Y_ASSERT(Array[i].size() > j);
        res = &Array[i][j];
    }
};
