#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/stream/output.h>
#include <util/stream/buffer.h>
#include <util/generic/vector.h>
#include <util/charset/wide.h>

#include <kernel/search_daemon_iface/relevance_type.h>
#include <kernel/factor_storage/factor_storage.h>

struct TInfoParams;

TString FormatFloat(float val, bool allowExponential = true, int precision = 10);
TString FormatDouble(double val, bool allowExponential = true, int precision = 10);
TString BeautifyFactorValue(const TString& str);

enum ERowType {
    RT_DATA      = 0,
    RT_HEADER    = 1,
    RT_PROBLEM   = 2,
    RT_SUBHEADER = 3
};

struct TInfoDataCell {
    TString Text;
    bool Encode = false;

    TInfoDataCell() = default;

    explicit TInfoDataCell(TString text, bool encode = true)
        : Text(std::move(text))
        , Encode(encode)
    {
    }
};

struct IInfoWriter {
    virtual ~IInfoWriter() = default;

    virtual void AddError(const TString& error) = 0;
    virtual void StartResponse() = 0;
    virtual void EndResponse() = 0;

    virtual void StartTable(const char* tableName) = 0;
    virtual void EndTable() = 0;
    virtual void AddRow(ERowType rowType, const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10, bool encode) = 0;
    virtual void AddRow(const TVector<TInfoDataCell>& cells, ERowType rowType) = 0;

    virtual void WriteTextTable(const char* tableName, const TString& data) = 0;
};

struct IInfoDataTable {
    virtual ~IInfoDataTable() = default;

    virtual void AddRow(ERowType rowType, const TString& c1, const TString& c2=TString(), const TString& c3=TString(), const TString& c4=TString(), const TString& c5=TString(), const TString& c6=TString(), const TString& c7=TString(), const TString& c8=TString(), const TString& c9=TString(), const TString& c10=TString()) = 0;
    virtual void AddRow(                  const TString& c1, const TString& c2=TString(), const TString& c3=TString(), const TString& c4=TString(), const TString& c5=TString(), const TString& c6=TString(), const TString& c7=TString(), const TString& c8=TString(), const TString& c9=TString(), const TString& c10=TString()) = 0;
    virtual void AddNonEncodedRow(        const TString& c1, const TString& c2=TString(), const TString& c3=TString(), const TString& c4=TString(), const TString& c5=TString(), const TString& c6=TString(), const TString& c7=TString(), const TString& c8=TString(), const TString& c9=TString(), const TString& c10=TString()) = 0;
    virtual void AddRow(const TVector<TInfoDataCell>& cells, ERowType rowType = RT_DATA) = 0;
};

using PtrInfoDataTable = TSimpleSharedPtr<IInfoDataTable>;

// Info request answer formatter
class TInfoRequestFormatter: TNonCopyable {
private:
    THolder<IInfoWriter> Writer;

    bool Started = false;
    bool UnclosedDataTable = false;
    bool Error = false;
    ui32 PageCount = 1;
    ui32 PageNumber = 1;
    TRelevance DocRelevance = Max<TRelevance>();
    TFactorStorage Factors;

public:
    TInfoRequestFormatter(bool json, TBufferOutput& out);

    bool IsError() const;
    void AddError(const TString& errorMsg);

    // Adds data table to info request answer
    // Returned table is not valid after next call to AddDataTable
    PtrInfoDataTable AddDataTable(const char* tableName);

    void WriteTextTable(const char* tableName, const TString& data);

    void SetPageCount(ui32 c);
    void SetPageNumber(ui32 n);

    void SetDocRelevance(TRelevance relevance);
    void SetFactors(const TFactorStorage& Factors);

    void Finalize();

private:
    void Check();
    void AddDataTableInternal(const char* tableName);

    void AddMetaInfoDataTable();
};

void FormatInfoRequestError(bool json, const TString& error, TBufferStream& out);

static inline void TextToHTML(TUtf16String& w) {
    EscapeHtmlChars<true>(w);
}

static inline TString TextToHTML(const TString& s) {
    TUtf16String w = UTF8ToWide(s);
    TextToHTML(w);
    return WideToUTF8(w);
}
