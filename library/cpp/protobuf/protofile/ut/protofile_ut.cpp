#include <library/cpp/protobuf/protofile/protofile.h>
#include <library/cpp/protobuf/protofile/ut/protofile_ut.pb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>
#include <util/system/tempfile.h>

static float seed = 1.f;

static float FloatGen() {
    seed *= 3.5;
    seed += 3.14;
    if (seed > 1e8)
        seed /= 1000;
    return seed;
}

Y_UNIT_TEST_SUITE(ProtoFileTest) {
    static const char TempFileName[] = "./ProtoFile-test";

    template <template <class...> class TWriter, class TMessage>
    void Write(const TString& fileName, const size_t messageCount) {
        TProtoTest message;
        TFixedBufferFileOutput output(fileName);
        TWriter<TMessage> writerImpl;
        NFastTier::IProtoWriter<TMessage>& writer = writerImpl;
        writer.Open(&output);
        TString randomCrap = "Lorem ipsum dot sir amet, и съешь ещё этих мягких французских булок! ";
        for (size_t i = 0; i < messageCount; ++i) {
            TString tmp = randomCrap;
            message.Clear();

            message.SetHash(i * i * 5000000);
            for (size_t j = 0; j < (i % 800) + 10; ++j)
                message.AddFactors(FloatGen());
            for (size_t j = 0; j < (i % 8) + 3; ++j) {
                tmp += randomCrap + ToString(i);
                message.AddBlob(tmp);
            }
            if (i % 3)
                message.SetVersion(ui32(i));
            writer.Write(message);
        }
        writer.Finish();
    }

    template <template <class...> class TReader, class TMessage>
    void Read(const TString& fileName, const size_t messageCount) {
        TProtoTest message;
        TFileInput input(fileName);
        TReader<TMessage> readerImpl;
        NFastTier::IProtoReader<TMessage>& reader = readerImpl;
        reader.Open(&input);
        TString randomCrap = "Lorem ipsum dot sir amet, и съешь ещё этих мягких французских булок! ";
        for (size_t i = 0; i < messageCount; ++i) {
            UNIT_ASSERT_EQUAL(reader.GetNext(message), true);
            TString tmp = randomCrap;
            UNIT_ASSERT_EQUAL(message.GetHash(), i * i * 5000000);
            for (size_t j = 0; j < (i % 800) + 10; ++j)
                UNIT_ASSERT_EQUAL(message.GetFactors(j), FloatGen());
            for (size_t j = 0; j < (i % 8) + 3; ++j) {
                tmp += randomCrap + ToString(i);
                UNIT_ASSERT_EQUAL(message.GetBlob(j), tmp);
            }
            if (i % 3) {
                UNIT_ASSERT_EQUAL(message.GetVersion(), ui32(i));
            } else {
                UNIT_ASSERT_EQUAL(message.HasVersion(), false);
            }
        }
        UNIT_ASSERT_EQUAL(reader.GetNext(message), false);
    }

    template <template <class...> class TWriter, template <class...> class TReader, class TMessage>
    void RunTest(const size_t messageCount) {
        TTempFile tmpFile(TempFileName);
        seed = 1.0f;
        Write<TWriter, TMessage>(tmpFile.Name(), messageCount);
        seed = 1.0f;
        Read<TReader, TMessage>(tmpFile.Name(), messageCount);
    }

    Y_UNIT_TEST(TestBinary) {
        RunTest<NFastTier::TBinaryProtoWriter, NFastTier::TBinaryProtoReader, TProtoTest>(1000);
    }

    Y_UNIT_TEST(TestText) {
        RunTest<NFastTier::TTextProtoWriter, NFastTier::TTextProtoReader, TProtoTest>(200);
    }

    Y_UNIT_TEST(TestGeneralBinary) {
        RunTest<NFastTier::TBinaryProtoWriter, NFastTier::TBinaryProtoReader, ::google::protobuf::Message>(1000);
    }

    Y_UNIT_TEST(TestGeneralText) {
        RunTest<NFastTier::TTextProtoWriter, NFastTier::TTextProtoReader, ::google::protobuf::Message>(200);
    }
};
