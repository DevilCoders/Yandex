#pragma once

#include <kernel/info_request/inforequestformatter.h>

#include <library/cpp/html/pcdata/pcdata.h>

#include <util/stream/output.h>

namespace NSnippets {

struct IDebugTable {
    virtual void AddNonEncodedRowImpl(const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10) = 0;
    virtual void AddRowImpl(const TVector<TInfoDataCell>& cells, ERowType rowType) = 0;
    virtual void AddNonEncodedRow(const TString& c1, const TString& c2 = TString(), const TString& c3 = TString(), const TString& c4 = TString(), const TString& c5 = TString(), const TString& c6 = TString(), const TString& c7 = TString(), const TString& c8 = TString(), const TString& c9 = TString(), const TString& c10 = TString()) {
        AddNonEncodedRowImpl(c1, c2, c3, c4, c5, c6, c7, c8, c9, c10);
    }
    virtual void AddRow(const TVector<TInfoDataCell>& cells, ERowType rowType = RT_DATA) {
        AddRowImpl(cells, rowType);
    }
    virtual ~IDebugTable();
};

struct IDebugFormatter {
    //valid until next AddDataTable
    virtual IDebugTable* AddDataTable(const char* tableName) = 0;
    virtual void SetPageCount(ui32 c) = 0;
    virtual void SetPageNumber(ui32 n) = 0;
    virtual void Finalize() = 0;
    virtual ~IDebugFormatter();
};

struct TInfoDebugTable : IDebugTable {
    PtrInfoDataTable Impl;
    void AddNonEncodedRowImpl(const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10) override {
        Impl->AddNonEncodedRow(c1, c2, c3, c4, c5, c6, c7, c8, c9, c10);
    }
    void AddRowImpl(const TVector<TInfoDataCell>& cells, ERowType rowType) override {
        Impl->AddRow(cells, rowType);
    }
};

struct TSerpDebugFormatter : IDebugFormatter {
    TInfoRequestFormatter Impl;
    TInfoDebugTable Tbl;
    TSerpDebugFormatter(bool isJson, TBufferOutput& out)
      : Impl(isJson, out)
    {
    }
    IDebugTable* AddDataTable(const char* tableName) override {
        Tbl.Impl = Impl.AddDataTable(tableName);
        return &Tbl;
    }
    void SetPageCount(ui32 c) override {
        Impl.SetPageCount(c);
    }
    void SetPageNumber(ui32 n) override {
        Impl.SetPageNumber(n);
    }
    void Finalize() override {
        Impl.Finalize();
    }
};

struct TTabDebugFormatter : IDebugTable, IDebugFormatter {
    TBufferOutput& Out;
    TTabDebugFormatter(TBufferOutput& out)
      : Out(out)
    {
    }
    IDebugTable* AddDataTable(const char*) override {
        return this;
    }
    void SetPageCount(ui32) override {
    }
    void SetPageNumber(ui32) override {
    }
    void AddNonEncodedRowImpl(const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10) override {
        TVector<TInfoDataCell> cells;
        cells.push_back(TInfoDataCell(c1));
        cells.push_back(TInfoDataCell(c2));
        cells.push_back(TInfoDataCell(c3));
        cells.push_back(TInfoDataCell(c4));
        cells.push_back(TInfoDataCell(c5));
        cells.push_back(TInfoDataCell(c6));
        cells.push_back(TInfoDataCell(c7));
        cells.push_back(TInfoDataCell(c8));
        cells.push_back(TInfoDataCell(c9));
        cells.push_back(TInfoDataCell(c10));
        while (cells.size() > 1 && !cells.back().Text.size()) {
            cells.pop_back();
        }
        AddRowImpl(cells, RT_DATA);
    }
    void AddRowImpl(const TVector<TInfoDataCell>& cells, ERowType) override {
        for (size_t i = 0; i < cells.size(); ++i) {
            if (i) {
                Out << '\t';
            }
            if (!cells[i].Encode || true)
                Out << cells[i].Text;
            else {
                // get rid of broken characters
                TUtf16String w = UTF8ToWide<true>(cells[i].Text);
                TString s = WideToUTF8(w);
                Out << EncodeHtmlPcdata(s.data());
            }
        }
        Out << Endl;
    }
    void Finalize() override {
    }
};

}
