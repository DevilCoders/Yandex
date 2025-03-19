#include "inforequestformatter.h"

#include <library/cpp/html/pcdata/pcdata.h>

#include <util/generic/yexception.h>
#include <util/generic/buffer.h>
#include <util/charset/wide.h>

#include <library/cpp/json/writer/json.h>

template <class T>
static TString FormatNumber(T val, bool allowExponential, int precision) {
    if (val == -::Max<T>()) {
        return TString("−∞");
    } else if (val == ::Max<T>()) {
        return TString("+∞");
    }

    char buf[64];
    if (val == 0)
        return "0";
    if (val == 1)
        return "1";

    sprintf(buf, allowExponential ? "%.*g" : "%.*f", precision, val);
    return buf;
}

TString FormatFloat(float val, bool allowExponential, int precision) {
    return FormatNumber<float>(val, allowExponential, precision);
}

TString FormatDouble(double val, bool allowExponential, int precision) {
    return FormatNumber<double>(val, allowExponential, precision);
}

// Truncate extra zeroes and decimal point sign
TString BeautifyFactorValue(const TString& str) {
    if (TStringBuf("0") == str)
        return str;

    TString result = str;
    // TODO: replace this shit with proper StripStringRight
    // StripStringRight(result, EqualsStripAdapter('0'));
    while (result.size() && result.back() == '0')
        result.pop_back();
    if (result.size() && result.back() == '.')
        result.pop_back();
    return result;
}

static TString FixString(const TString& str) {
    // get rid of broken characters
    TUtf16String w = UTF8ToWide<true>(str);
    return WideToUTF8(w);
}

static TString FormatCell(const TString& str, bool encode) {
    if (!encode)
        return str;

    TString s = FixString(str);
    return EncodeHtmlPcdata(s.c_str());
}

/// This should not throw as it will be used in dtor
static void FinalizeErrors(const TVector<TString> errors, TBufferOutput& out) {
    if (!errors) {
        return;
    }

    try {
        out.Buffer().Clear();
        out << "Info request was processed with the following errors:\n";
        for (const auto& error : errors) {
            out << "ERROR: " << error << '\n';
        }
    } catch (...) {
        Cerr << "An error occured while serializing errors: " << CurrentExceptionMessage() << Endl;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

class TInfoXmlWriter : public IInfoWriter {
private:
    TBufferOutput& Out;
    TVector<TString> Errors;

public:
    TInfoXmlWriter(TBufferOutput& out);
    ~TInfoXmlWriter() override {
        FinalizeErrors(Errors, Out);
    }

    void AddError(const TString& errorMsg) override;
    void StartResponse() override;
    void EndResponse() override;

    void StartTable(const char* tableName) override;
    void EndTable() override;
    void AddRow(ERowType rowType, const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10, bool encode) override;
    void AddRow(const TVector<TInfoDataCell>& cells, ERowType rowType) override;

    void WriteTextTable(const char* tableName, const TString& data) override;

private:
    void OpenRow(ERowType rowType);
    void AddCell(const TString& c, ui8 n, bool encode);
    void CloseRow();
};

TInfoXmlWriter::TInfoXmlWriter(TBufferOutput& out)
    : Out(out)
{
}

void TInfoXmlWriter::AddError(const TString& errorMsg) {
    Errors.push_back(errorMsg);
}

void TInfoXmlWriter::StartResponse() {
    Out << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n<response>\n";
}

void TInfoXmlWriter::EndResponse() {
    Out << "</response>";
}

void TInfoXmlWriter::StartTable(const char* tableName) {
    Out << "<i1 n=\"" << tableName << "\">";
}

void TInfoXmlWriter::EndTable() {
    Out << "</i1>\n";
}

void TInfoXmlWriter::AddRow(ERowType rowType, const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10, bool encode) {
    OpenRow(rowType);
    AddCell(c1, 1, encode);
    AddCell(c2, 2, encode);
    AddCell(c3, 3, encode);
    AddCell(c4, 4, encode);
    AddCell(c5, 5, encode);
    AddCell(c6, 6, encode);
    AddCell(c7, 7, encode);
    AddCell(c8, 8, encode);
    AddCell(c9, 9, encode);
    AddCell(c10, 10, encode);
    CloseRow();
}

void TInfoXmlWriter::AddRow(const TVector<TInfoDataCell>& cells, ERowType rowType) {
    OpenRow(rowType);
    int idx = 1;
    for (TVector<TInfoDataCell>::const_iterator ii = cells.begin(), end = cells.end(); ii != end; ++ii) {
        AddCell(ii->Text, idx++, ii->Encode);
    }
    CloseRow();
}

void TInfoXmlWriter::OpenRow(ERowType rowType) {
    Out << "<i2 t=\"" << ui32(rowType) << '\"';
}

void TInfoXmlWriter::AddCell(const TString& c, ui8 n, bool encode) {
    if (!!c)
        Out << " c" << ToString(n) << "=\"" << FormatCell(c, encode) << "\"";
}

void TInfoXmlWriter::CloseRow() {
    Out << "/>";
}

void TInfoXmlWriter::WriteTextTable(const char* tableName, const TString& data) {
    StartTable(tableName);
    Out << data;
    EndTable();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

class TInfoJsonWriter : public IInfoWriter {
private:
    TBufferOutput& Out;
    NJsonWriter::TBuf JsonWriter;
    TVector<TString> Errors;

public:
    TInfoJsonWriter(TBufferOutput& out);
    ~TInfoJsonWriter() override {
        FinalizeErrors(Errors, Out);
    };

    void AddError(const TString& errorMsg) override;
    void StartResponse() override;
    void EndResponse() override;

    void StartTable(const char* tableName) override;
    void EndTable() override;
    void AddRow(ERowType rowType, const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10, bool encode) override;
    void AddRow(const TVector<TInfoDataCell>& cells, ERowType rowType) override;

    void WriteTextTable(const char* tableName, const TString& data) override;

private:
    void OpenRow(ERowType rowType);
    ui8 CalcCellCount(const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10);
    void AddCell(const TString& c, bool encode);
    void CloseRow();
};

TInfoJsonWriter::TInfoJsonWriter(TBufferOutput& out)
    : Out(out)
    , JsonWriter(NJsonWriter::HEM_DONT_ESCAPE_HTML, &out)
{
}

void TInfoJsonWriter::AddError(const TString& errorMsg) {
    Errors.push_back(errorMsg);
}

void TInfoJsonWriter::StartResponse() {
    JsonWriter.BeginObject();
}

void TInfoJsonWriter::EndResponse() {
    JsonWriter.EndObject();
}

void TInfoJsonWriter::StartTable(const char* tableName) {
    JsonWriter.WriteKey(tableName);
    JsonWriter.BeginObject();

    JsonWriter.WriteKey("type");
    JsonWriter.WriteString("table");

    JsonWriter.WriteKey("rows");
    JsonWriter.BeginList();
}

void TInfoJsonWriter::EndTable() {
    JsonWriter.EndList();
    JsonWriter.EndObject();
}

void TInfoJsonWriter::AddRow(ERowType rowType, const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10, bool encode) {
    OpenRow(rowType);

    ui8 c = CalcCellCount(c1, c2, c3, c4, c5, c6, c7, c8, c9, c10);
    if (c >= 1)
        AddCell(c1, encode);
    if (c >= 2)
        AddCell(c2, encode);
    if (c >= 3)
        AddCell(c3, encode);
    if (c >= 4)
        AddCell(c4, encode);
    if (c >= 5)
        AddCell(c5, encode);
    if (c >= 6)
        AddCell(c6, encode);
    if (c >= 7)
        AddCell(c7, encode);
    if (c >= 8)
        AddCell(c8, encode);
    if (c >= 9)
        AddCell(c9, encode);
    if (c >= 10)
        AddCell(c10, encode);

    CloseRow();
}

void TInfoJsonWriter::AddRow(const TVector<TInfoDataCell>& cells, ERowType rowType) {
    OpenRow(rowType);

    for (TVector<TInfoDataCell>::const_iterator ii = cells.begin(), end = cells.end(); ii != end; ++ii) {
        AddCell(ii->Text, ii->Encode);
    }

    CloseRow();
}

void TInfoJsonWriter::OpenRow(ERowType rowType) {
    JsonWriter.BeginObject();

    JsonWriter.WriteKey("t");
    JsonWriter.WriteString(ToString(ui32(rowType)));

    JsonWriter.WriteKey("cells");
    JsonWriter.BeginList();
}

ui8 TInfoJsonWriter::CalcCellCount(const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10) {
    if (c10)
        return 10;
    if (c9)
        return 9;
    if (c8)
        return 8;
    if (c7)
        return 7;
    if (c6)
        return 6;
    if (c5)
        return 5;
    if (c4)
        return 4;
    if (c3)
        return 3;
    if (c2)
        return 2;
    if (c1)
        return 1;
    return 0;
}

void TInfoJsonWriter::AddCell(const TString& c, bool /*encode*/) {
    JsonWriter.WriteString(FixString(c));
}

void TInfoJsonWriter::CloseRow() {
    JsonWriter.EndList();
    JsonWriter.EndObject();
}

void TInfoJsonWriter::WriteTextTable(const char* tableName, const TString& data) {
    JsonWriter.WriteKey(tableName);
    JsonWriter.BeginObject();

    JsonWriter.WriteKey("type");
    JsonWriter.WriteString("text");

    JsonWriter.WriteKey("data");
    JsonWriter.WriteString(data);

    JsonWriter.EndObject();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

class TInfoDataTable : public IInfoDataTable, TNonCopyable {
private:
    IInfoWriter* Writer = nullptr;

public:
    TInfoDataTable(IInfoWriter* writer);

    void AddRow(ERowType rowType, const TString& c1, const TString& c2=TString(), const TString& c3=TString(), const TString& c4=TString(), const TString& c5=TString(), const TString& c6=TString(), const TString& c7=TString(), const TString& c8=TString(), const TString& c9=TString(), const TString& c10=TString()) override;
    void AddRow(                  const TString& c1, const TString& c2=TString(), const TString& c3=TString(), const TString& c4=TString(), const TString& c5=TString(), const TString& c6=TString(), const TString& c7=TString(), const TString& c8=TString(), const TString& c9=TString(), const TString& c10=TString()) override;
    void AddNonEncodedRow(        const TString& c1, const TString& c2=TString(), const TString& c3=TString(), const TString& c4=TString(), const TString& c5=TString(), const TString& c6=TString(), const TString& c7=TString(), const TString& c8=TString(), const TString& c9=TString(), const TString& c10=TString()) override;
    void AddRow(const TVector<TInfoDataCell>& cells, ERowType rowType = RT_DATA) override;

private:
    void AddRowInternal(ERowType rowType, const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10, bool encode);
};

TInfoDataTable::TInfoDataTable(IInfoWriter* writer)
    : Writer(writer)
{
}

void TInfoDataTable::AddRowInternal(ERowType rowType, const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10, bool encode) {
    Writer->AddRow(rowType, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, encode);
}

void TInfoDataTable::AddRow(ERowType rowType, const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10) {
    AddRowInternal(rowType, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, false);
}

void TInfoDataTable::AddRow(const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10) {
    AddRowInternal(RT_DATA, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, false);
}

void TInfoDataTable::AddNonEncodedRow(const TString& c1, const TString& c2, const TString& c3, const TString& c4, const TString& c5, const TString& c6, const TString& c7, const TString& c8, const TString& c9, const TString& c10) {
    AddRowInternal(RT_DATA, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, true);
}

void TInfoDataTable::AddRow(const TVector<TInfoDataCell>& cells, ERowType rowType) {
    Writer->AddRow(cells, rowType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

static IInfoWriter* CreateInfoWriter(bool json, TBufferOutput& out) {
    if (json)
        return new TInfoJsonWriter(out);
    else
        return new TInfoXmlWriter(out);
}

TInfoRequestFormatter::TInfoRequestFormatter(bool json, TBufferOutput& out)
    : Writer(CreateInfoWriter(json, out))
{
}

bool TInfoRequestFormatter::IsError() const {
    return Error;
}

void TInfoRequestFormatter::AddError(const TString& errorMsg) {
    Writer->AddError(errorMsg);
    Error = true;
}

void TInfoRequestFormatter::Check() {
    if (!Started) {
        Writer->StartResponse();
        Started = true;
    }

    if (UnclosedDataTable)
        Writer->EndTable();
}

void TInfoRequestFormatter::AddDataTableInternal(const char* tableName) {
    Check();

    Writer->StartTable(tableName);

    UnclosedDataTable = true;
}

PtrInfoDataTable TInfoRequestFormatter::AddDataTable(const char* tableName) {
    AddDataTableInternal(tableName);
    return PtrInfoDataTable(new TInfoDataTable(Writer.Get()));
}

void TInfoRequestFormatter::WriteTextTable(const char* tableName, const TString& data) {
    Check();
    return Writer->WriteTextTable(tableName, data);
}

void TInfoRequestFormatter::SetPageCount(ui32 c) {
    PageCount = c;
}

void TInfoRequestFormatter::SetPageNumber(ui32 n) {
    PageNumber = n;
}

void TInfoRequestFormatter::SetDocRelevance(TRelevance relevance) {
    DocRelevance = relevance;
}

void TInfoRequestFormatter::SetFactors(const TFactorStorage& factors) {
    Factors = factors;
}

void TInfoRequestFormatter::Finalize() {
    if (Error)
        return;

    AddMetaInfoDataTable();

    if (UnclosedDataTable)
        Writer->EndTable();

    Writer->EndResponse();
}

void TInfoRequestFormatter::AddMetaInfoDataTable() {
    if (PageCount == 1 && DocRelevance == Max<TRelevance>())
        return;

    PtrInfoDataTable table = AddDataTable("MetaData");

    if (PageCount != 1) {
        table->AddRow("PageCount",  ToString(PageCount));
        table->AddRow("PageNumber", ToString(PageNumber));
    }

    if (DocRelevance != Max<TRelevance>()) {
        table->AddRow("Relevance", ToString(DocRelevance));

        if (Factors.Size()) {
            TStringStream factorsStr;

            TMultiConstFactorView multiView =
                Factors.CreateMultiConstViewFor(ESliceRole::MAIN, ESliceRole::META);

            for (auto view : multiView) {
                for (size_t i = 0; i < view.Size(); ++i) {
                    if (!factorsStr.Empty()) {
                        factorsStr << ' ';
                    }
                    factorsStr << FormatFloat(view[i]);
                }
            }

            table->AddRow("Factors", factorsStr.Str());
        }
    }
}

void FormatInfoRequestError(bool json, const TString& error, TBufferStream& out) {
    TInfoRequestFormatter formatter(json, out);
    formatter.AddError(error);
    formatter.Finalize();
    out.Flush();
}
