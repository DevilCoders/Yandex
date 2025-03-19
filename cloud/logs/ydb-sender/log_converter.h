#pragma once

#include "ydb_writer.h"

#include <kikimr/persqueue/sdk/deprecated/cpp/v2/persqueue.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <logfeller/lib/log_parser/factory.h>
#include <logfeller/lib/chunk_splitter/chunk_splitter.h>
#include <logfeller/lib/parsing/config_storage/config_storage.h>

#include <util/generic/vector.h>

class TLogConverter {
public:
    using TColInfo = TYdbWriter::TColInfo;
    using TTimestamp = i64;
    using TLogRow = NYdb::TValue;

public:
    TLogConverter(TString topic, TDuration newTableInterval, TString timestampColumn);

    const THashMap<TString, TColInfo>& GetColumnList() const;

    // Groups parsed rows by table name
    void Parse(const NPersQueue::TReadResponse& msg, THashMap<TString, TVector<TLogRow>>& batches, TVector<TLogConverter::TLogRow>& streams);

    // Returns table name and row
    std::pair<TString, TLogRow> ParseRecord(TStringBuf record, NLogFeller::NChunkSplitter::TRecordContext context, TString& err, TString nowRfc3339) noexcept;

    std::pair<TString, TString> extractStream(TLogConverter::TLogRow row) const;

private:
    // Based on timestamp (and possibly other fields) decides which table the row should be written to
    TString MakeTableName(TTimestamp ts, const NYT::TNode& parsed) const;
    TLogConverter::TLogRow makeStreamRow(std::pair<TString, TString> streamName, TString nowRfc3339) const;

private:
    NLogFeller::NChunkSplitter::TChunkSplitterPtr Splitter;
    NLogFeller::NLogParser::TLogParserPtr Parser;
    NLogFeller::NYTCommon::TSchema Schema;

    THashMap<TString, TColInfo> Columns;
    TDuration NewTableInterval;
    TString TimestampColumn;
};
