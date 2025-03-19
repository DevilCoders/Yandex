#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <kernel/embeddings_info/dssm_embeddings.h>
#include <kernel/embeddings_info/embedding.h>
#include <kernel/embeddings_info/embedding_traits.h>

#include <util/generic/string.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>

#include <array>

struct TEmbeddingInfo {
    NDssmApplier::EDssmDocEmbedding EmbeddingId;
    NDssmApplier::EDssmDocEmbeddingType EmbeddingType;
    TStringBuf EmbeddingsFileName;

    constexpr TEmbeddingInfo(NDssmApplier::EDssmDocEmbedding embeddingId, NDssmApplier::EDssmDocEmbeddingType embeddingType, const TStringBuf& embeddingsFileName)
        : EmbeddingId(embeddingId)
        , EmbeddingType(embeddingType)
        , EmbeddingsFileName(embeddingsFileName)
    {
    }
};

namespace {
    const float NormDiffEps = 1e-3;
    static constexpr std::array<TEmbeddingInfo, 3> EmbeddingsInfo = {
        TEmbeddingInfo(NDssmApplier::EDssmDocEmbedding::CompressedUserRecDssmSpyTitleDomain, NDssmApplier::EDssmDocEmbeddingType::CompressedUserRecDssmSpyTitleDomain, TStringBuf("compressed_user_rec_dssm_spy_title_domain_test.txt")),
        TEmbeddingInfo(NDssmApplier::EDssmDocEmbedding::CompressedUrlRecDssmSpyTitleDomain, NDssmApplier::EDssmDocEmbeddingType::CompressedUrlRecDssmSpyTitleDomain, TStringBuf("compressed_url_rec_dssm_spy_title_domain_test.txt")),
        TEmbeddingInfo(NDssmApplier::EDssmDocEmbedding::UncompressedCFSharp, NDssmApplier::EDssmDocEmbeddingType::UncompressedCFSharp, TStringBuf("uncompressed_cf_sharp_test.txt"))
    };
}

float EmbeddingsNormDiff(const TVector<float>& left, const TVector<float>& right) {
    UNIT_ASSERT(left.size() == right.size());

    float diff = 0;
    for (size_t i = 0; i != left.size(); ++i) {
        diff += (left[i] - right[i]) * (left[i] - right[i]);
    }

    return diff;
}

void ReadEmbeddingsFromFile(NDssmApplier::EDssmDocEmbedding embeddingId, const TFsPath& fileName, TVector<NDocDssmEmbeddings::TEmbedding>* embeddings) {
    TFileInput file(TFsPath(ArcadiaSourceRoot() + "/kernel/embeddings_info/ut/" + fileName));
    TString line;

    const NDssmApplier::TDocEmbeddingTransferTraits& transferTraits = NDssmApplier::DocEmbeddingsTransferTraits.at(ToUnderlying(embeddingId));

    for (size_t i = 0; file.ReadLine(line) != 0; ++i) {
        TVector<float> embedding;
        StringSplitter(line).Split(' ').SkipEmpty().ParseInto(&embedding);

        try {
            embeddings->push_back(NDocDssmEmbeddings::TEmbedding(embedding, embeddingId));
        } catch(...) {
            UNIT_ASSERT_C(false, TStringBuilder() << "FAIL: " << ToString(transferTraits.Type) << " embedding test size ("<< embedding.size() << ") in line " << i << " doesn't macth to expectedEmbedSize (" << transferTraits.RequiredEmbeddingSize << ')');
        }
    }
}

void CheckCompressionPrecision(NDssmApplier::EDssmDocEmbedding embeddingId, NDssmApplier::EDssmDocEmbeddingType embeddingType, const TVector<NDocDssmEmbeddings::TEmbedding>& embeddings) {
    for (const NDocDssmEmbeddings::TEmbedding& unprocessedEmbedding : embeddings) {
        NDocDssmEmbeddings::TEmbedding processedEmbedding(unprocessedEmbedding.Serialize(embeddingType), embeddingId);

        UNIT_ASSERT_C(EmbeddingsNormDiff(unprocessedEmbedding.Coordinates, processedEmbedding.Coordinates) < NormDiffEps, TStringBuilder() << "FAIL: " << ToString(embeddingType) << " NormDiff is too big");
    }
}

Y_UNIT_TEST_SUITE(TEmbeddingTest) {
    Y_UNIT_TEST(TEmptyEmbeddingTest) {
        for (const TEmbeddingInfo& embInfo : EmbeddingsInfo) {
            const TString serialized = "";
            const TVector<float> embedding;
            NDocDssmEmbeddings::TEmbedding emb(serialized, embInfo.EmbeddingId);
            UNIT_ASSERT_EQUAL_C(serialized, emb.Serialize(embInfo.EmbeddingType), TStringBuilder() << "FAIL: Empty serialized of " << ToString(embInfo.EmbeddingType) << " are not equal");
            UNIT_ASSERT_EQUAL_C(embedding, emb.Coordinates, TStringBuilder() << "FAIL: Empty coordinates of " << ToString(embInfo.EmbeddingType) << " are not equal");
        }
    }

    Y_UNIT_TEST(TNonEmptyEmbeddingTest) {
        for (const TEmbeddingInfo& embInfo : EmbeddingsInfo) {
            TVector<NDocDssmEmbeddings::TEmbedding> embeddings;
            ReadEmbeddingsFromFile(embInfo.EmbeddingId, embInfo.EmbeddingsFileName, &embeddings);
            CheckCompressionPrecision(embInfo.EmbeddingId, embInfo.EmbeddingType, embeddings);
        }
    }
}
