#include "part_compressed_ext.h"

#include <library/cpp/codecs/codecs.h>

#include <library/cpp/testing/unittest/gtest.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NRTYArchive {
    namespace {
        const TStringBuf TestDocs[] = {
            "first line",
            "second line",
            "line after the first and the second lines"
        };

        void ExpectEqualBlobs(const TBlob& b1, const TBlob& b2) {
            TStringBuf s1{b1.AsCharPtr(), b1.Size()};
            TStringBuf s2{b2.AsCharPtr(), b2.Size()};
            EXPECT_EQ(s1, s2);
        }

        TEST(TArchivePartCompressedExtSuite, TBlobVector) {
            NCodecs::TTrivialCodec codec;
            TBlobVectorBuilder builder(codec, 10000, 1000);
            EXPECT_TRUE(builder.Empty());
            EXPECT_EQ(builder.GetCount(), 0);
            EXPECT_TRUE(builder.GetCopy(1).Empty());
            const size_t docCount = std::end(TestDocs) - std::begin(TestDocs);
            for (size_t i = 0; i < docCount; ++i) {
                builder.Put(TBlob::NoCopy(TestDocs[i].data(), TestDocs[i].size()));
                EXPECT_EQ(builder.GetCount(), i + 1);
                EXPECT_FALSE(builder.IsFull());
            }
            TBuffer buffer;
            builder.SaveToBuffer(buffer);
            TBlobVector blobVector(codec, TBlob::FromBuffer(buffer));
            EXPECT_EQ(blobVector.GetCount(), docCount);
            for (size_t i = 0; i < docCount; ++i) {
                ExpectEqualBlobs(blobVector.GetCopy(i), builder.GetCopy(i));
            }

            TBlob b1 = blobVector.Take(1);
            ExpectEqualBlobs(b1, builder.GetCopy(1));
            EXPECT_EQ(blobVector.GetCount(), 0);

            builder.Clear();
            EXPECT_TRUE(builder.Empty());
            EXPECT_EQ(builder.GetCount(), 0);
            const auto lonelyBlob = TBlob::FromString("lonely_blob");
            builder.Put(lonelyBlob);
            EXPECT_EQ(builder.GetCount(), 1);
            ExpectEqualBlobs(builder.GetCopy(0), lonelyBlob);

            TBuffer buffer2;
            builder.SaveToBuffer(buffer2);
            TBlobVector vector2(codec);
            vector2.LoadFromBlob(TBlob::FromBuffer(buffer2));
            EXPECT_EQ(vector2.GetCount(), 1);
            ExpectEqualBlobs(vector2.GetCopy(0), lonelyBlob);
        }

        TEST(TArchivePartCompressedExtSuite, TBlobVectorEmpty) {
            NCodecs::TTrivialCodec codec;
            TBlobVectorBuilder builder(codec, 10000, 1000);

            TBuffer buffer;
            builder.SaveToBuffer(buffer);
            TBlobVector emptyVector(codec, TBlob::FromBuffer(buffer));
            EXPECT_EQ(emptyVector.GetCount(), 0);
            EXPECT_TRUE(emptyVector.GetCopy(0).Empty());
        }

        TEST(TArchivePartCompressedExtSuite, TBlobVectorSingleEmptyElement) {
            NCodecs::TTrivialCodec codec;
            TBlobVectorBuilder builder(codec, 10000, 1000);
            builder.Put({});

            TBuffer buffer;
            builder.SaveToBuffer(buffer);
            TBlobVector emptyVector(codec, TBlob::FromBuffer(buffer));
            EXPECT_EQ(emptyVector.GetCount(), 1);
            EXPECT_TRUE(emptyVector.GetCopy(0).Empty());
        }

        TEST(TArchivePartCompressedExtSuite, TBlobVectorLZ4) {
            const auto testBlob = TBlob::FromString("testing lz4 codec");
            auto codec = NCodecs::ICodec::GetInstance("lz4");
            TBlobVectorBuilder builder(*codec, 10000, 1000);
            builder.Put(testBlob);

            TBuffer buffer;
            builder.SaveToBuffer(buffer);
            TBlobVector blobVector(*codec, TBlob::FromBuffer(buffer));
            EXPECT_EQ(blobVector.GetCount(), 1);
            ExpectEqualBlobs(blobVector.GetCopy(0), testBlob);
        }

        TEST(TArchivePartCompressedExtSuite, TCodecLearnQueueSmallData) {
            const auto testBlob = TBlob::FromString("trying to learn on small data. should fall back to TTrivialCodec");
            auto codec = NCodecs::ICodec::GetInstance("zstd08d-1");
            auto lz4 = NCodecs::ICodec::GetInstance("lz4");
            TCodecLearnQueue learnQueue(codec, 0, 1000);
            learnQueue.PutBlob(testBlob);
            ExpectEqualBlobs(learnQueue.GetBlobCopy(0), testBlob);
            EXPECT_TRUE(learnQueue.GetBlobCopy(1).Empty());
            EXPECT_FALSE(learnQueue.IsFull());

            TBuffer buffer;
            auto finalCodec = learnQueue.TrainCodec(buffer);
            EXPECT_EQ(finalCodec->GetName(), lz4->GetName());

            TFirstBlock firstBlock(TBlob::FromBuffer(buffer));
            EXPECT_EQ(firstBlock.GetCodec()->GetName(), lz4->GetName());
            ExpectEqualBlobs(firstBlock.GetBlobCopy(0), testBlob);
        }

        TEST(TArchivePartCompressedExtSuite, TCodecLearnQueueDumpEmpty) {
            const auto testBlob = TBlob::FromString("trying to learn on small data. should fall back to TTrivialCodec");
            auto lz4 = NCodecs::ICodec::GetInstance("lz4");
            TCodecLearnQueue learnQueue(lz4, 0, 1000);

            TBuffer buffer;
            auto finalCodec = learnQueue.TrainCodec(buffer);
            EXPECT_EQ(finalCodec->GetName(), lz4->GetName());

            TFirstBlock firstBlock(TBlob::FromBuffer(buffer));
            EXPECT_EQ(firstBlock.GetCodec()->GetName(), lz4->GetName());
            EXPECT_EQ(firstBlock.GetCount(), 0);
            ExpectEqualBlobs(firstBlock.GetBlobCopy(0), {});
        }

        TEST(TArchivePartCompressedExtSuite, TCodecLearnQueueZstdDict) {
            const size_t testLearnSize = TCodecLearnQueue::MinimalLearnSize + 10;
            const TStringBuf fragment = " repeat this until learn size is reached. ";
            TBuffer buffer(testLearnSize + fragment.size());
            int i = 0;
            while (buffer.Size() < testLearnSize) {
                buffer.Append(fragment.data(), fragment.size());
                auto strCounter = ToString(++i);
                buffer.Append(strCounter.data(), strCounter.size());
            }
            const TBlob testBlob = TBlob::FromBuffer(buffer);
            auto codec = NCodecs::ICodec::GetInstance("zstd08d-1");
            TCodecLearnQueue learnQueue(codec, 0, 1000);
            learnQueue.PutBlob(testBlob);
            ExpectEqualBlobs(learnQueue.GetBlobCopy(0), testBlob);
            EXPECT_TRUE(learnQueue.GetBlobCopy(1).Empty());
            EXPECT_TRUE(learnQueue.IsFull());

            TBuffer firstBlockBuffer;
            auto finalCodec = learnQueue.TrainCodec(firstBlockBuffer);
            EXPECT_EQ(finalCodec->GetName(), codec->GetName());

            TFirstBlock firstBlock(TBlob::FromBuffer(firstBlockBuffer));
            EXPECT_EQ(firstBlock.GetCodec()->GetName(), codec->GetName());
            ExpectEqualBlobs(firstBlock.GetBlobCopy(0), testBlob);
        }
    }
}

