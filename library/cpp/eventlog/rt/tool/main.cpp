#include <library/cpp/eventlog/rt/proto_lib/test_events.ev.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/eventlog/logparser.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/system/tempfile.h>

#include <search/idl/events.ev.pb.h>

static const char* wcmLogFile = "./wcmlog";

template <typename T>
T CreateTestMessage(ui32 seed) {
    T result;
    result.SetString(TStringBuilder() << "string_" << seed);
    result.SetUint32(seed + 1);
    result.MutableOptionalNestedMessage()->SetUint64(seed + 2);

    for (size_t i = 0; i < 2; ++i) {
        result.AddRepeatedNestedMessage()->SetUint64(seed + 2 + i);
    }

    return result;
}

TString FilterEvents(const TString& fileName, TIntrusivePtr<TEventFilter> evFilter, ui64 startTime, ui64 endTime, bool strongOrdering,
                     IFrameFilterRef frameFilter, TEvent::TOutputFormat outputFormat) {
    TFrameStreamer frameStream(fileName, startTime, endTime, MAX_REQUEST_DURATION, NEvClass::Factory(), frameFilter);
    TEventStreamer eventStream(frameStream, startTime, endTime, strongOrdering, evFilter);

    TString buf;
    TStringBuilder ret;

    while (eventStream.Avail()) {
        const TEvent& ev = **eventStream;

        buf.clear();
        TStringOutput out(buf);

        ev.Print(out, outputFormat);
        out.Flush();

        ret << "    " << buf << "\n";

        eventStream.Next();
    }
    return TString(ret);
}

TString FilterEventsSafe(const TString& fileName, TIntrusivePtr<TEventFilter> evFilter, ui64 startTime, ui64 endTime, bool strongOrdering,
                     IFrameFilterRef frameFilter, TEvent::TOutputFormat outputFormat) {
    TString res;
    try {
        res = FilterEvents(fileName, evFilter, startTime, endTime, strongOrdering, frameFilter, outputFormat);
    }
    catch (...) {
        res = CurrentExceptionMessage();
    }
    return res;
}

void CreateEventLog(const TString& fileName) {
    TEventLog eventLog(fileName, NEvClass::Factory()->CurrentFormat());

    TEventLogFrame frame(eventLog);
    frame.LogEvent(1247677303028161ULL, NEvClass::TPeerName("11.22.33.44"));
    frame.LogEvent(1247677303028261ULL, NEvClass::TContextCreated("url", "reqid", 123456));
    frame.LogEvent(1247677303028361ULL, NEvClass::TStopMerge::FromFields(5, 6));
    frame.LogEvent(1247677303028461ULL, NEvClass::TCloseConnection::FromFields(10, 21));
    frame.Flush();

    frame.LogEvent(1247677303028362ULL, NEvClass::TWaitFinishSameReqs(456, "1111111111Z"));
    frame.LogEvent(1247677303028462ULL, NEvClass::TSaveToCache(false, true));
    frame.LogEvent(1247677303028662ULL, NEvClass::TServerStopped("host", 12345, 1111));
    frame.Flush();

    {
        TSelfFlushLogFrame sfFrame(eventLog);
        sfFrame.LogEvent(1247677303028065ULL, NEvClass::TCommonParseError(1, "name", 2));
        sfFrame.LogEvent(1247677303028064ULL, NEvClass::TSubSourceError(2, 3, 4, 0, "status", "cont", 0, "group#", 1, 0.75, -1, "", 0));
        sfFrame.LogEvent(1247677303028063ULL, NEvClass::TReqWizardStartProcessingRules());
    }

    static constexpr TStringBuf bytes1 = "BYTES/VALUE";
    frame.LogEvent(
        1247677303029000ULL,
        NEvClass::TAllFieldTypesTestMessage(
            -1,
            -2,
            3,
            4,
            5.5,
            6.6,
            true,
            "STRING/VALUE",
            NEvClass::AllFieldTypesTestMessage_enum_type::AllFieldTypesTestMessage_enum_type_SECOND_VAL,
            CreateTestMessage<NEvClass::MessageWithMessageId>(0),
            CreateTestMessage<NEvClass::MessageWithoutMessageId>(1),
            bytes1.data(), bytes1.size()
        )
    );
    frame.Flush();

    frame.LogEvent(1247677303029001ULL, NEvClass::TStringFieldMessage("OneSlashAtEnd/"));
    frame.Flush();

    frame.LogEvent(1247677303029002ULL, NEvClass::TStringFieldMessage("TwoSlashesAtEnd//"));
    frame.Flush();

    frame.LogEvent(1247677303029003ULL, NEvClass::TStringFieldMessage("Slash/InTheMiddle"));
    frame.Flush();

    frame.LogEvent(1247677303029004ULL, NEvClass::TStringFieldMessage("NoSlash"));
    frame.Flush();

    frame.LogEvent(1247677303029005ULL, NEvClass::TStringFieldMessage("OneBackslashAtEnd\\"));
    frame.LogEvent(1247677303029006ULL, NEvClass::TStringFieldMessage("TwoBackslashesAtEnd\\\\"));
    frame.LogEvent(1247677303029007ULL, NEvClass::TStringFieldMessage("DifferentNoSlash"));
    frame.Flush();

    frame.LogEvent(1247677303029008ULL, NEvClass::TStringFieldMessage("Colon:InTheMiddle"));
    frame.Flush();

    {
        NEvClass::TAllRepeatedFieldTypesTestMessage message(
            -2,
            -3,
            4,
            5,
            6.5,
            7.6,
            true,
            "STRING/VALUE",
            NEvClass::AllRepeatedFieldTypesTestMessage_enum_type::AllRepeatedFieldTypesTestMessage_enum_type_SECOND_VAL,
            CreateTestMessage<NEvClass::MessageWithMessageId>(0),
            CreateTestMessage<NEvClass::MessageWithoutMessageId>(1),
            bytes1.data(), bytes1.size()
        );

        static constexpr TStringBuf bytes2 = "BYTES/VALUE/V2";
        message.MergeFrom(
            NEvClass::TAllRepeatedFieldTypesTestMessage(
                -3,
                -4,
                5,
                6,
                7.5,
                8.6,
                false,
                "STRING/VALUE/V2",
                NEvClass::AllRepeatedFieldTypesTestMessage_enum_type::AllRepeatedFieldTypesTestMessage_enum_type_FIRST_VAL,
                CreateTestMessage<NEvClass::MessageWithMessageId>(2),
                CreateTestMessage<NEvClass::MessageWithoutMessageId>(3),
                bytes2.data(), bytes2.size()
            )
        );

        frame.LogEvent(1247677303029009ULL, message);
        frame.Flush();
    }

    eventLog.CloseLog();
}

void TestOrdered(TEvent::TOutputFormat outputFormat) {
    TIntrusivePtr<TEventFilter> filter(new TEventFilter(false));
    filter->AddEventClass(0);
    TString res = FilterEvents(wcmLogFile, filter, MIN_START_TIME, MAX_END_TIME, /*strongOrdering = */ true, nullptr, outputFormat);
    Cout << "  TestOrdered:" << Endl;
    Cout << res << Endl;
}

void TestTimeIntervals(TEvent::TOutputFormat outputFormat) {
    TIntrusivePtr<TEventFilter> filter(new TEventFilter(false));
    filter->AddEventClass(0);
    TString res = FilterEvents(wcmLogFile, filter, 1247677303028361ULL, 1247677303028562ULL, /*strongOrdering = */ true, nullptr, outputFormat);

    Cout << "  TestTimeIntervals:" << Endl;
    Cout << res << Endl;
}

void TestIncludeEvents(TEvent::TOutputFormat outputFormat) {
    TIntrusivePtr<TEventFilter> filter(new TEventFilter(true));
    filter->AddEventClass(111);
    filter->AddEventClass(276);
    filter->AddEventClass(305);
    filter->AddEventClass(287);
    filter->AddEventClass(336);
    TString res = FilterEvents(wcmLogFile, filter, MIN_START_TIME, MAX_END_TIME, /*strongOrdering = */ true, nullptr, outputFormat);

    Cout << "  TestIncludeEvents" << Endl;
    Cout << res << Endl;
}

void TestExcludeEvents(TEvent::TOutputFormat outputFormat) {
    TIntrusivePtr<TEventFilter> filter(new TEventFilter(false));
    filter->AddEventClass(303);
    filter->AddEventClass(264);
    filter->AddEventClass(272);
    filter->AddEventClass(343);
    filter->AddEventClass(0);
    TString res = FilterEvents(wcmLogFile, filter, MIN_START_TIME, MAX_END_TIME, /*strongOrdering = */ true, nullptr, outputFormat);

    Cout << "  TestExcludeEvents:" << Endl;
    Cout << res << Endl;
}

void TestFrameFilters(TEvent::TOutputFormat outputFormat) {
    IFrameFilterRef frameFilter = new TFrameIdFrameFilter(3);
    TString res = FilterEvents(wcmLogFile, /* eventFilter =*/nullptr, MIN_START_TIME, MAX_END_TIME, /*strongOrdering = */ true, frameFilter, outputFormat);

    Cout << "  TestFrameFilters:" << Endl;
    Cout << res << Endl;
}

void TestContainsEventFrameFilter(TEvent::TOutputFormat outputFormat) {
    TString fieldTypeSupportInitString = "AllFieldTypesTestMessage:Int32:-1/"
                         "AllFieldTypesTestMessage:Int64:-2/"
                         "AllFieldTypesTestMessage:Uint32:3/"
                         "AllFieldTypesTestMessage:Uint64:4/"
                         "AllFieldTypesTestMessage:Float:5.5/"
                         "AllFieldTypesTestMessage:Double:6.6/"
                         "AllFieldTypesTestMessage:Bool:1/"
                         "AllFieldTypesTestMessage:String:STRING\\/VALUE/"
                         "AllFieldTypesTestMessage:Enum:SECOND_VAL";

    IFrameFilterRef fieldTypeSupportFrameFilter = new TContainsEventFrameFilter(fieldTypeSupportInitString, NEvClass::Factory());

    TString resFieldTypeSupport = FilterEvents(wcmLogFile, /* eventFilter = */ nullptr, MIN_START_TIME, MAX_END_TIME, /* strongOrdering = */ true, fieldTypeSupportFrameFilter, outputFormat);


    TString nonexFieldTypeInitString = "AllFieldTypesTestMessage:NonexistentType:ValueNeverChecked";

    IFrameFilterRef nonexFieldTypeFrameFilter = new TContainsEventFrameFilter(nonexFieldTypeInitString, NEvClass::Factory());

    TString resNonexFieldType = FilterEventsSafe(wcmLogFile, /* eventFilter = */ nullptr, MIN_START_TIME, MAX_END_TIME, /* strongOrdering = */ true, nonexFieldTypeFrameFilter, outputFormat);

    TVector<TString> slashEscapingInitStrings = {"StringFieldMessage:String:OneSlashAtEnd\\/",
                                                 "StringFieldMessage:String:TwoSlashesAtEnd\\/\\/",
                                                 "StringFieldMessage:String:Slash\\/InTheMiddle",
                                                 "StringFieldMessage:String:NoSlash/",
                                                 "StringFieldMessage:String:NoSlash//",
                                                 "StringFieldMessage:String:OneBackslashAtEnd\\\\/"
                                                 "StringFieldMessage:String:TwoBackslashesAtEnd\\\\\\\\/"
                                                 "StringFieldMessage:String:DifferentNoSlash"};

    TVector<TString> resSlashEscaping;
    for (const TString& initString : slashEscapingInitStrings) {
        IFrameFilterRef slashEscapingFrameFilter = new TContainsEventFrameFilter(initString, NEvClass::Factory());
        resSlashEscaping.push_back(FilterEvents(wcmLogFile, /* eventFilter = */ nullptr, MIN_START_TIME, MAX_END_TIME, /* strongOrdering = */ true, slashEscapingFrameFilter, outputFormat));
    }

    TString colonEscapingInitString = "StringFieldMessage:String:Colon\\:InTheMiddle";

    IFrameFilterRef colonEscapingFrameFilter = new TContainsEventFrameFilter(colonEscapingInitString, NEvClass::Factory());

    TString resColonEscaping = FilterEvents(wcmLogFile, /* eventFilter = */ nullptr, MIN_START_TIME, MAX_END_TIME, /* strongOrdering = */ true, colonEscapingFrameFilter, outputFormat);

    Cout << "  TestContainsEventFrameFilter:" << Endl;
    Cout << "Field type support test:" << Endl;
    Cout << resFieldTypeSupport << Endl;
    Cout << "Nonexistent field type test:" << Endl;
    Cout << resNonexFieldType << Endl << Endl;
    Cout << "Slash escaping test:" << Endl;
    for (const TString& resultString : resSlashEscaping) {
        Cout << resultString << Endl;
    }
    Cout << "Colon escaping test:" << Endl;
    Cout << resColonEscaping << Endl;
}

int main() {
    try {
        for (size_t i = 0; i < 2; ++i) {
            TTempFile tempFile(wcmLogFile);
            CreateEventLog(wcmLogFile);

            TVector<std::function<void(TEvent::TOutputFormat)>> tests = {
                TestOrdered,
                TestTimeIntervals,
                TestIncludeEvents,
                TestExcludeEvents,
                TestFrameFilters,
                TestContainsEventFrameFilter,
            };

            for (TEvent::TOutputFormat outputFormat : {TEvent::TOutputFormat::TabSeparated, TEvent::TOutputFormat::Json}) {
                Cout << "OutputFormat[" << i << "] " << outputFormat << Endl;
                for (auto test : tests) {
                    test(outputFormat);
                }
            }
        }
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        abort();
    }

    return 0;
}
