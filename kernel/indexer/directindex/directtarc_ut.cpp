#include <library/cpp/charset/wide.h>
#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/digest/old_crc/crc.h>
#include <util/generic/buffer.h>
#include <util/stream/zlib.h>

#include <kernel/tarc/iface/tarcface.h>
#include <kernel/tarc/markup_zones/text_markup.h>

#include "directtarc.h"

//#ifdef UNIT_ASSERT_STRINGS_EQUAL
//#undef UNIT_ASSERT_STRINGS_EQUAL
//#endif
//#define UNIT_ASSERT_STRINGS_EQUAL(a, b) Cout << "    UNIT_ASSERT_STRINGS_EQUAL(\"" << b << "\", " << #b << ");" << Endl;

class TDirectTarcTest: public TTestBase {
    UNIT_TEST_SUITE(TDirectTarcTest);
        UNIT_TEST(Test);
    UNIT_TEST_SUITE_END();
public:
    void Test();
private:
    void AddText(NIndexerCore::TArcTextBlockCreator& creator, const TUtf16String& text, const char* zoneName = nullptr) {
        creator.AddText(text.c_str(), text.size(), zoneName);
    }
};

void TDirectTarcTest::Test() {
    TBuffer outBuf;

    {
        NIndexerCore::TArcTextBlockCreator creator;
        creator.OpenDocument();
        AddText(creator, u"Заголовок!", "title");
        AddText(creator, u"Принцип восприятия философски ассоциирует \tконфликт, несмотря на мнение авторитетов.");
        creator.CommitDocument(&outBuf);
    }

    UNIT_ASSERT_STRINGS_EQUAL("155", ToString(outBuf.Size()).data());
    ui64 c = crc64(outBuf.Data(), outBuf.Size());
    UNIT_ASSERT_STRINGS_EQUAL("13775301340935161351", ToString(c).data());

    const ui8* data = (const ui8*)outBuf.Data();
    TArchiveTextHeader& hdr = *((TArchiveTextHeader*)data);
    data += sizeof(TArchiveTextHeader);

    UNIT_ASSERT_STRINGS_EQUAL("1", ToString(hdr.BlockCount).data());
    UNIT_ASSERT_STRINGS_EQUAL("23", ToString(hdr.InfoLen).data());

    TArchiveMarkupZones mZones;
    UnpackMarkupZones(data, hdr.InfoLen, &mZones);
    TArchiveZone& zTitle = mZones.GetZone(AZ_TITLE);
    UNIT_ASSERT_STRINGS_EQUAL("1", ToString(zTitle.Spans.size()).data());
    const TArchiveZoneSpan& tSpan = zTitle.Spans[0];
    UNIT_ASSERT_STRINGS_EQUAL("1", ToString(tSpan.SentBeg).data());
    UNIT_ASSERT_STRINGS_EQUAL("1", ToString(tSpan.SentEnd).data());
    UNIT_ASSERT_STRINGS_EQUAL("0", ToString(tSpan.OffsetBeg).data());
    UNIT_ASSERT_STRINGS_EQUAL("9", ToString(tSpan.OffsetEnd).data());

    data += hdr.InfoLen;

    TArchiveTextBlockInfo* blockInfos = (TArchiveTextBlockInfo*)data;
    const TArchiveTextBlockInfo& b = blockInfos[0];
    data += hdr.BlockCount * sizeof(TArchiveTextBlockInfo);

    UNIT_ASSERT_STRINGS_EQUAL("2", ToString(b.SentCount).data());
    TBufferOutput block;
    {
        TMemoryInput in(data, b.EndOffset);
        TZLibDecompress decompressor(&in);
        TransferData(&decompressor, &block);
    }
    UNIT_ASSERT_STRINGS_EQUAL("128", ToString(block.Buffer().Size()).data());
    TMemoryInput in(block.Buffer().Data(), block.Buffer().Size());
    TArchiveWeightZones::Skip(&in);

    TVector<char> tempBuffer;
    TStringBuf sentInfos;

    LoadArchiveTextBlockSentInfos(&in, &sentInfos, &tempBuffer);
    UNIT_ASSERT(sentInfos.size() % sizeof(TArchiveTextBlockSentInfo) == 0);

    const TArchiveTextBlockSentInfo* sis = (TArchiveTextBlockSentInfo*)sentInfos.data();
    const ui32 sentInfoCount = sentInfos.size() / sizeof(TArchiveTextBlockSentInfo);

    const char* sentBlob = nullptr;
    const size_t sentBlobSize = in.Next(&sentBlob);
    UNIT_ASSERT(in.Exhausted());

    UNIT_ASSERT_STRINGS_EQUAL("2", ToString(sentInfoCount).data());

    TString s((const char*)sentBlob, 0, sis[0].EndOffset);
    UNIT_ASSERT_STRINGS_EQUAL(s.data(), WideToChar(u"Заголовок!", CODES_YANDEX).c_str());
    sentBlob += sis[0].EndOffset;
    UNIT_ASSERT(sis[0].EndOffset < sentBlobSize);
    s = TString((const char*)sentBlob, 0, sis[1].EndOffset - sis[0].EndOffset);
    UNIT_ASSERT_STRINGS_EQUAL(s.data(), WideToChar(u"Принцип восприятия философски ассоциирует конфликт, несмотря на мнение авторитетов.", CODES_YANDEX).c_str());
    UNIT_ASSERT(sis[1].EndOffset == sentBlobSize);
}

UNIT_TEST_SUITE_REGISTRATION(TDirectTarcTest);
