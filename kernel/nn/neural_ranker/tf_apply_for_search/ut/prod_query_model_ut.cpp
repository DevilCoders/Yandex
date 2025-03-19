#include <library/cpp/testing/unittest/registar.h>
#include <kernel/nn/tf_apply/applier.h>
#include <kernel/nn/neural_ranker/protos/meta_info/meta_info.pb.h>
#include <kernel/nn/neural_ranker/concat_features/features_concater.h>
#include <util/stream/file.h>
#include <util/generic/maybe.h>


Y_UNIT_TEST_SUITE(TTFProdModel) {
    Y_UNIT_TEST(RunSimple) {
        NNeuralRankerProtocol::TRankModel queryRank;
        TFileInput queryInputStream("query_meta.pb");
        queryRank.ParseFromArcadiaStream(&queryInputStream);

        NNeuralRankerProtocol::TInput queryInput = queryRank.GetMetaInfo().GetInputs(0);

        TMaybe<NTFModel::TTFGraphDef> queryGraph;
        queryGraph.ConstructInPlace();
        queryGraph->ParseFromString(queryRank.GetGraphSerialized());

        NTFModel::TTFApplier<NTFModel::TTFDense1DInput<float>> queryApplier(
            *queryGraph,
            queryRank.GetMetaInfo().GetOutputVectorLen(),
            queryRank.GetMetaInfo().GetOutputNodeName()
        );

        NNeuralRanker::TFeaturesConcater queryConcater(queryInput);

        TString queryInputNodeName = queryRank.GetMetaInfo().GetInputNodeNames(0);
        size_t querySize = NNeuralRanker::TFeaturesConcater::GetRawVectorLen(queryInput);

        TVector<float> sample(querySize, 0);

        NTFModel::TTFDense1DInput<float> q(sample);
        NTFModel::TTFInputNode<NTFModel::TTFDense1DInput<float>> queryNode({queryInputNodeName, &q});
        const auto& queryEmbedded = queryApplier.Run(&queryNode);

        TVector<float> reference{
            -0.024864, -0.0586071, 0.0231475, -0.0275167, 0.133561, 0.0998365, -0.0688, -0.0226379, -0.128538, -0.142286, -0.00183578, 0.36127, -0.134416, -0.0256864, -0.0945955, 0.178026, 0.361692, 0.0809368, -0.128795, -0.128198, 0.147415, 0.164596, -0.0462393, 0.255268, 0.0052456, -0.126612, 0.11691, -0.0419878, 0.0080675, 0.0109349, -0.107603, -0.125144, -0.0441929, 0.0184143, -0.114111, -0.0599193, 0.0409331, -0.0844505, -0.0913502, 0.0507462, 0.035415, -0.0260431, -0.132395, -0.0789264, -0.0805072, -0.0498198, -0.00914636, 0.167837, 0.285276, 0.123849, 0.0327259, 0.0124083, 0.102834, 0.0666037, 0.0335962, -0.132641, 0.267652, -0.0918088, 0.119773, -0.15723, -0.118174, -0.0234427, -0.101069, 0.111515
        };

        const auto& pred = queryEmbedded.Data[0];

        UNIT_ASSERT(pred.size() == reference.size());

        for (size_t i = 0; i < reference.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i], reference[i], 0.00001);
        }
    }
};
