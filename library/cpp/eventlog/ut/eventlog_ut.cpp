#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/eventlog/logparser.h>
#include <library/cpp/eventlog/eventlog_int.h>
#include <library/cpp/eventlog/ut/test_events.ev.pb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>
#include <util/system/tempfile.h>
#include <util/string/split.h>

using namespace NEvClass;

Y_UNIT_TEST_SUITE(EventLogTest) {
    template <TEvent::TOutputFormat Format, typename T>
    static void CheckEventOutput(T && proto, const TString& expected) {
        THolder<TEvent> ev(NEvClass::Factory()->CreateLogEvent(T::descriptor()->options().GetExtension(message_id)));
        TStringStream binary;
        Y_VERIFY(proto.SerializeToArcadiaStream(&binary));
        ev->Load(binary);

        TStringStream out;
        ev->Print(out, Format);
        TString result = out.Str();
        TStringBuf shortResult = result;
        shortResult.NextTok('\t');
        shortResult.NextTok('\t');
        TStringBuf className = shortResult.NextTok('\t');

        UNIT_ASSERT_VALUES_EQUAL(className, T::descriptor()->name());
        UNIT_ASSERT_VALUES_EQUAL(shortResult, expected);
    }

    template <typename T>
    static void CheckEventOutputTsv(T&& event, const TString& expected) {
        CheckEventOutput<TEvent::TOutputFormat::TabSeparated>(std::move(event), expected);
    }

    template <typename T>
    static void CheckEventOutputTsvRaw(T&& event, const TString& expected) {
        CheckEventOutput<TEvent::TOutputFormat::TabSeparatedRaw>(std::move(event), expected);
    }

    Y_UNIT_TEST(TestEscapedChars) {
        for (auto p : TVector<std::tuple<TOneField, TString, TString>>{
                 {{""}, "", ""},
                 {{"\n"}, "\\n", "\n"},
                 {{"\t"}, "\t", "\t"},
                 {{"\\"}, "\\\\", "\\"},
                 {{"\na"}, "\\na", "\na"},
                 {{"a\n"}, "a\\n", "a\n"},
                 {{"\ta"}, "\ta", "\ta"},
                 {{"a\t"}, "a\t", "a\t"},
                 {{"\\a"}, "\\\\a", "\\a"},
                 {{"a\\"}, "a\\\\", "a\\"},
                 {{"\n\\"}, "\\n\\\\", "\n\\"},
                 {{"\\\n"}, "\\\\\\n", "\\\n"},
                 {{"\n\n"}, "\\n\\n", "\n\n"},
                 {{"\\\\"}, "\\\\\\\\", "\\\\"}}) {
            CheckEventOutputTsv(std::get<0>(p), std::get<1>(p));
            CheckEventOutputTsvRaw(std::get<0>(p), std::get<2>(p));
        }

        for (auto p : TVector<std::tuple<TTwoFields, TString, TString>>{
                 {{"500", "a\nb\nc"}, "500\ta\\nb\\nc", "500\ta\nb\nc"},
                 {{"500", "a\\nb\\nc"}, "500\ta\\\\nb\\\\nc", "500\ta\\nb\\nc"},
                 {{"500", "a\tb\tc"}, "500\ta\tb\tc", "500\ta\tb\tc"},
                 {{"500", "a\tb\nc\td"}, "500\ta\tb\\nc\td", "500\ta\tb\nc\td"},
                 {{"a\tb", "c\td"}, "a\tb\tc\td", "a\tb\tc\td"},
                 {{"a\nb", "c\nd"}, "a\\nb\tc\\nd", "a\nb\tc\nd"},
                 {{"\t", "\n"}, "\t\t\\n", "\t\t\n"},
                 {{"\n", "\t"}, "\\n\t\t", "\n\t\t"},
                 {{"\n", "\n"}, "\\n\t\\n", "\n\t\n"},
                 {{"\t", "\t"}, "\t\t\t", "\t\t\t"}}) {
            CheckEventOutputTsv(std::get<0>(p), std::get<1>(p));
            CheckEventOutputTsvRaw(std::get<0>(p), std::get<2>(p));
        }
    }

    void TestReadWrite(TEventLogFormat format) {
        auto tempFile = TTempFile(MakeTempName());
        auto tempFileName = tempFile.Name();

        const size_t frameCount = 100;

        {
            TEventLog eventLog(tempFileName, NEvClass::Factory()->CurrentFormat(), {}, format);

            for (size_t i = 1; i < frameCount; ++i) {
                TSelfFlushLogFrame frame(eventLog);
                TString s = ToString(i);
                for (size_t j = 0; j < i; ++j) {
                    if (j & 1) {
                        frame.LogEvent(TOneField(s));
                    } else {
                        frame.LogEvent(TTwoFields(s, s));
                    }
                }
            }
        }

        TFileInput fi(tempFileName);
        TFrameStreamer log(fi, NEvClass::Factory());
        size_t i = 1;
        while (log.Avail()) {
            const TFrame& frame = *log;

            auto it = frame.GetIterator();
            for (size_t j = 0; j < i; ++j) {
                UNIT_ASSERT(it.Avail());
                TStringStream ss;
                (*it)->Print(ss);

                TVector<TString> fields;
                Split(ss.Str(), "\t", fields);

                if (j & 1) {
                    UNIT_ASSERT_VALUES_EQUAL(fields.size(), 4);
                    UNIT_ASSERT_STRINGS_EQUAL(fields[2], "TOneField");
                    UNIT_ASSERT_STRINGS_EQUAL(fields[3], ToString(i));
                } else {
                    UNIT_ASSERT_VALUES_EQUAL(fields.size(), 5);
                    UNIT_ASSERT_STRINGS_EQUAL(fields[2], "TTwoFields");
                    UNIT_ASSERT_STRINGS_EQUAL(fields[3], ToString(i));
                    UNIT_ASSERT_STRINGS_EQUAL(fields[4], ToString(i));
                }
                it.Next();
            }
            ++i;
            log.Next();
        }

        UNIT_ASSERT_VALUES_EQUAL(i, frameCount);
    }

    Y_UNIT_TEST(TestReadWriteV4) {
        TestReadWrite(COMPRESSED_LOG_FORMAT_V4);
    }
    Y_UNIT_TEST(TestReadWriteV5) {
        TestReadWrite(COMPRESSED_LOG_FORMAT_V5);
    }
    Y_UNIT_TEST(TestReadFromSteamWithGarbageBetweenFrames) {
        TFileInput fi("frames_with_newlines_between_them");
        TFrameStreamer log(fi, NEvClass::Factory());
        size_t count = 0;
        for (; log.Avail(); log.Next()) {
            ++count;
        }

        UNIT_ASSERT_VALUES_EQUAL(count, 56);
    }

    Y_UNIT_TEST(TestMetaFlags) {
        class TTestLogBackendStub: public TLogBackend {
        public:
            TTestLogBackendStub(TLogRecord::TMetaFlags& data)
                : Data_(data)
            {
            }

            void WriteData(const TLogRecord& record) override {
                Data_ = record.MetaFlags;
            }

            void ReopenLog() override {
            }

        private:
            TLogRecord::TMetaFlags& Data_;
        };

        TLogRecord::TMetaFlags metaFlags;

        TEventLog eventLog(TLog(MakeHolder<TTestLogBackendStub>(metaFlags)), NEvClass::Factory()->CurrentFormat());

        {
            TSelfFlushLogFrame frame(eventLog);
            frame.AddMetaFlag("key1", "value1");
            frame.AddMetaFlag("key2", "value2");
            frame.LogEvent(TOneField("some data"));
        }

        TLogRecord::TMetaFlags expected{{"key1", "value1"}, {"key2", "value2"}};
        UNIT_ASSERT_EQUAL(metaFlags, expected);
    }
}
