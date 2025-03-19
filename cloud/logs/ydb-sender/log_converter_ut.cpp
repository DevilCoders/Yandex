#include "log_converter.h"
#include "ydb_writer.h"

#include <ydb/public/sdk/cpp/client/ydb_value/value.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>

#include <iostream>

Y_UNIT_TEST_SUITE(TLogConverterTest) {
    Y_UNIT_TEST(TestSchema) {
        TLogConverter converter("yc-log", TDuration::Seconds(1), nullptr);
        const THashMap<TString, TLogConverter::TColInfo>& columnMap = converter.GetColumnList();
        TStringStream str;
        UNIT_ASSERT_VALUES_EQUAL(NYdb::EPrimitiveType::String, columnMap.at("logGroupId").Type);
        UNIT_ASSERT_VALUES_EQUAL(NYdb::EPrimitiveType::String, columnMap.at("streamName").Type);
        UNIT_ASSERT_VALUES_EQUAL(NYdb::EPrimitiveType::String, columnMap.at("ksuid").Type);
        UNIT_ASSERT_VALUES_EQUAL(NYdb::EPrimitiveType::String, columnMap.at("createdAt").Type);
        UNIT_ASSERT_VALUES_EQUAL(NYdb::EPrimitiveType::String, columnMap.at("ingestedAt").Type);
        UNIT_ASSERT_VALUES_EQUAL(NYdb::EPrimitiveType::String, columnMap.at("savedAt").Type);
        UNIT_ASSERT_VALUES_EQUAL(NYdb::EPrimitiveType::String, columnMap.at("message").Type);

        //            for (auto cit = columnMap.begin(); cit != columnMap.end(); ++cit) {
        //                if (cit->first != "_logfeller_timestamp" && cit->first != "iso_eventtime") {
        //                    NYdb::EPrimitiveType type = cit->second.Type;
        //                    str << "    " << cit->first << " " << type;
        //                    str << ",\n";
        //                }
        //            }
        //            std::cout << str.Str();
    }
    Y_UNIT_TEST(TestParseRecord) {
        TLogConverter converter("yc-log", TDuration::Days(2), nullptr);
        NLogFeller::NChunkSplitter::TRecordContext context;
        TString err;
        TString now("2019-03-13T17:20:00.000Z");
        const THashMap<TString, TLogConverter::TColInfo>& columnMap = converter.GetColumnList();
        std::pair<TString, TLogConverter::TLogRow> parsed = converter.ParseRecord("{\"group_id\": \"group id\",\"stream_name\":\"stream   name\",\"ksuid\": \"KSUID\",\"message\": \"whaaa\\nsss\", \"created_at\":\"2006-01-02T15:04:05.999999999+07:00\", \"ingested_at\": \"2006-01-02T15:04:05-07:33\", \"saved_at\": \"2006-01-02T15:04:05-07:33\"}",
                                                                                  context, err, now);
        UNIT_ASSERT_VALUES_EQUAL("log__2006-01-02", parsed.first);
        auto valueParser = NYdb::TValueParser(parsed.second);
        valueParser.OpenStruct();
        while (valueParser.TryNextMember()) {
            const auto& paramName = valueParser.GetMemberName();
            const auto& columnName = paramName.c_str() + 3;
            UNIT_ASSERT(columnMap.contains(columnName));
            valueParser.OpenOptional();
            UNIT_ASSERT(!valueParser.IsNull());
            UNIT_ASSERT_VALUES_EQUAL(NYdb::EPrimitiveType::String, valueParser.GetPrimitiveType());
            const TString& stringValue = valueParser.GetString();
            valueParser.CloseOptional();
            if (strcmp(columnName, "logGroupId") == 0) {
                UNIT_ASSERT_VALUES_EQUAL(TString("group id"), stringValue);
            } else if (strcmp(columnName, "streamName") == 0) {
                UNIT_ASSERT_VALUES_EQUAL(TString("stream   name"), stringValue);
            } else if (strcmp(columnName, "ksuid") == 0) {
                UNIT_ASSERT_VALUES_EQUAL(TString("KSUID"), stringValue);
            } else if (strcmp(columnName, "message") == 0) {
                UNIT_ASSERT_VALUES_EQUAL(TString("whaaa\nsss"), stringValue);
            } else if (strcmp(columnName, "savedAt") == 0) {
                UNIT_ASSERT_VALUES_EQUAL(now, stringValue);
            } else if (strcmp(columnName, "createdAt") == 0) {
                UNIT_ASSERT_VALUES_EQUAL(TString("2006-01-02T15:04:05.999999999+07:00"), stringValue);
            } else if (strcmp(columnName, "ingestedAt") == 0) {
                UNIT_ASSERT_VALUES_EQUAL(TString("2006-01-02T15:04:05-07:33"), stringValue);
            } else {
                UNIT_FAIL(columnName);
            }
        }
        valueParser.CloseStruct();
    }

    Y_UNIT_TEST(TestParseBrokenRecord) {
        TLogConverter converter("yc-log", TDuration::Days(2), nullptr);
        NLogFeller::NChunkSplitter::TRecordContext context;
        TString err;
        TString now("2019-03-13T17:20:00.000Z");
        std::pair<TString, TLogConverter::TLogRow> parsed = converter.ParseRecord("{\"a\":\"b\"}",
                                                                                  context, err, now);
        std::cout << "Got error: " << err << std::endl;
        UNIT_ASSERT(!err.empty());
    }

    Y_UNIT_TEST(TestParse) {
        TLogConverter converter("yc-log", TDuration::Days(2), nullptr);
        NLogFeller::NChunkSplitter::TRecordContext context;
        TString err;
        TString now("2019-03-13T17:20:00.000Z");
        NPersQueue::TReadResponse response;
        auto* data = response.MutableData();
        auto* messageBatch = data->AddMessageBatch();
        auto *message = messageBatch->AddMessage();
        message->SetData("{\"group_id\": \"group1\",\"stream_name\":\"stream1\",\"ksuid\": \"KSUID1\",\"message\": \"whaaa\\nsss\", \"created_at\":\"2006-01-02T15:04:05.999999999+07:00\", \"ingested_at\": \"2006-01-02T15:04:05-07:33\", \"saved_at\": \"2006-01-02T15:04:05-07:33\"}");
        message = messageBatch->AddMessage();
        message->SetData("{\"group_id\": \"group1\", \"stream_name\":\"stream2\",\"ksuid\": \"KSUID2\",\"message\": \"blabla1\", \"created_at\":\"2006-01-02T15:04:05.999999999+07:00\", \"ingested_at\": \"2006-01-02T15:04:05-07:33\", \"saved_at\": \"2006-01-02T15:04:05-07:33\"}");
        message = messageBatch->AddMessage();
        message->SetData("{\"group_id\": \"group1\", \"stream_name\":\"stream1\",\"ksuid\": \"KSUID3\",\"message\": \"blabla2\", \"created_at\":\"2006-01-02T15:04:05.999999999+07:00\", \"ingested_at\": \"2006-01-02T15:04:05-07:33\", \"saved_at\": \"2006-01-02T15:04:05-07:33\"}");
        message = messageBatch->AddMessage();
        message->SetData("{\"group_id\": \"group2\", \"stream_name\":\"stream3\",\"ksuid\": \"KSUID3\",\"message\": \"blabla2\", \"created_at\":\"2006-01-02T15:04:05.999999999+07:00\", \"ingested_at\": \"2006-01-02T15:04:05-07:33\", \"saved_at\": \"2006-01-02T15:04:05-07:33\"}");

        THashMap<TString, TVector<TLogConverter::TLogRow>> batches;
        TVector<TLogConverter::TLogRow> streams;
        std::cout << "About to parse";
        converter.Parse(response, batches, streams);
        std::cout << "batches size" << batches.size() << std::endl;
        for (auto batch: batches) {
            std::cout << batch.first.c_str() << std::endl;
        }
        TVector<TLogConverter::TLogRow> &rows = batches.at("log__2006-01-02");
        UNIT_ASSERT_VALUES_EQUAL(4, rows.ysize());
        UNIT_ASSERT_VALUES_EQUAL(3, streams.ysize());
        THashSet<std::pair<TString, TString>> streamNames;
        for (auto s: streams) {
            streamNames.insert(converter.extractStream(s));
        }
        UNIT_ASSERT(streamNames.contains(std::make_pair("group1", "stream1")));
        UNIT_ASSERT(streamNames.contains(std::make_pair("group1", "stream2")));
        UNIT_ASSERT(streamNames.contains(std::make_pair("group2", "stream3")));

    }

    Y_UNIT_TEST(TestParseBrokenBatch) {
        TLogConverter converter("yc-log", TDuration::Days(2), nullptr);
        NLogFeller::NChunkSplitter::TRecordContext context;
        TString err;
        TString now("2019-03-13T17:20:00.000Z");
        NPersQueue::TReadResponse response;
        auto* data = response.MutableData();
        auto* messageBatch = data->AddMessageBatch();
        auto *message = messageBatch->AddMessage();
        message->SetData("{\"a\": \"b\"}");

        THashMap<TString, TVector<TLogConverter::TLogRow>> batches;
        TVector<TLogConverter::TLogRow> streams;
        std::cout << "About to parse";
        converter.Parse(response, batches, streams);
        UNIT_ASSERT_VALUES_EQUAL(0, batches.size());
    }
}
