#include <algorithm>

#include <kernel/dssm_applier/embeddings_transfer/embeddings_transfer.h>
#include <kernel/dssm_applier/embeddings_transfer/ut/applier/consts.h>
#include <kernel/dssm_applier/embeddings_transfer/ut/applier/protos/config.pb.h>

#include <library/cpp/getoptpb/getoptpb.h>

#include <util/generic/array_ref.h>
#include <util/stream/file.h>
#include <util/stream/str.h>

void write(TFileOutput &out, TConstArrayRef<float> emb) {
    for (const auto &v: emb)
        out << v << Endl;
}

void test(TFileOutput &out, const TVector<float> &emb, const TString &embeddingName, const TString &embeddingVersion) {
    NEmbeddingsTransfer::TEmbedding rawEmbedding(embeddingVersion, emb);
    NEmbeddingsTransfer::NProto::TModelEmbedding modelEmbedding;
    NEmbeddingsTransfer::SetEmbeddingData(modelEmbedding, embeddingName, rawEmbedding);
    TString str;
    Y_PROTOBUF_SUPPRESS_NODISCARD modelEmbedding.SerializeToString(&str);
    out << str;
    out << ResultsDelimiter << Endl;
    NEmbeddingsTransfer::NProto::TModelEmbedding decompressedModelEmbedding;
    Y_PROTOBUF_SUPPRESS_NODISCARD decompressedModelEmbedding.ParseFromString(str);
    auto decompressedEmbedding = NEmbeddingsTransfer::GetEmbeddingData(decompressedModelEmbedding);
    write(out, decompressedEmbedding.second.Data);
    out << TestsDelimiter << Endl;
}

int main(int argc, char const* argv[]) {
    TConfig config = NGetoptPb::GetoptPbOrAbort(argc, argv);
    TFileInput in(config.GetInput());
    TFileOutput out(config.GetOutput());
    int num_tests;
    in >> num_tests;
    TVector<TVector<float>> embeddings(num_tests);
    for (auto t = 0; t < num_tests; ++t) {
        int emb_size;
        in >> emb_size;
        embeddings[t].resize(emb_size);
        for (auto &value: embeddings[t])
            in >> value;
    }

    for_each(embeddings.begin(), embeddings.end(), [&out](TVector<float> emb) {
        test(out, emb, "dssm", "");
    });
    for_each(embeddings.begin(), embeddings.end(), [&out](TVector<float> emb) {
        test(out, emb, "query_bert_embed", "");
    });
    return 0;
}
