#include <library/cpp/testing/unittest/registar.h>
#include <kernel/nn/tf_apply/applier.h>
#include <util/random/random.h>

Y_UNIT_TEST_SUITE(TTFDynamicApplierTestSuite) {
    Y_UNIT_TEST(RunSimpleOneOneInputDynamic) {
        NTFModel::TTFDynamicApplier model("sum.pb", 1, "network_output");
        TVector<float> inputVector(2, 1.);
        NTFModel::TTFDense1DInput<float> simpleInput(inputVector);
        NTFModel::TTFDynamicInputNode node({{"input", &simpleInput},});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT_DOUBLES_EQUAL(pred[0][0], 2., 0.00001);
    }

    Y_UNIT_TEST(RunSimpleOneTwoInputs) {
        NTFModel::TTFDynamicApplier model("sum_2_inputs.pb", 1, "network_output");
        NTFModel::TTFDense1DInput<float> i(TVector<float>{1.,});
        NTFModel::TTFDense1DInput<float> j = i;
        UNIT_ASSERT(i.GetInputNames("input_1")[0] == "input_1");

        NTFModel::TTFDynamicInputNode node({{"input_1", &i}, {"input_2", &j}});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT_DOUBLES_EQUAL(pred[0][0], 2., 0.00001);
    }

    Y_UNIT_TEST(RunSimpleBatchOneInput) {
        NTFModel::TTFDynamicApplier model("sum.pb", 1, "network_output");
        size_t bs = 128;
        TVector<float> sample(2, 1.);
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < bs; ++i) {
            samples.push_back(sample);
        }
        NTFModel::TTFDense1DInput<float> simpleInput(samples, bs);
        NTFModel::TTFDynamicInputNode node({{"input", &simpleInput},});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        for (size_t i = 0; i < bs; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], 2., 0.00001);
        }
    }

    Y_UNIT_TEST(RunSimpleBatchTwoInputs) {
        NTFModel::TTFDynamicApplier model("sum_2_inputs.pb", 1, "network_output");
        size_t bs = 128;
        TVector<float> sample(1, 1.);
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < bs; ++i) {
            samples.push_back(sample);
        }

        NTFModel::TTFDense1DInput<float> i(samples, bs);
        NTFModel::TTFDense1DInput<float> j = i;
        NTFModel::TTFDynamicInputNode node({{"input_1", &i}, {"input_2", &j}});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        for (size_t i = 0; i < bs; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], 2., 0.00001);
        }
    }

    Y_UNIT_TEST(RunSimpleBatchesOneInput) {
        NTFModel::TTFDynamicApplier model("sum.pb", 1, "network_output");
        size_t testSize = 2000;
        size_t bs = 128;
        TVector<float> sample(2, 1.);
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < testSize; ++i) {
            samples.push_back(sample);
        }
        NTFModel::TTFDense1DInput<float> simpleInput(samples, bs);
        NTFModel::TTFDynamicInputNode node({{"input", &simpleInput},});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        for (size_t i = 0; i < testSize; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], 2., 0.00001);
        }
    }

    Y_UNIT_TEST(RunSimpleBatchesTwoInputs) {
        size_t testSize = 2000;
        NTFModel::TTFDynamicApplier model("sum_2_inputs.pb", 1, "network_output");
        size_t bs = 128;
        TVector<float> sample(1, 1.);
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < testSize; ++i) {
            samples.push_back(sample);
        }

        NTFModel::TTFDense1DInput<float> i(samples, bs);
        NTFModel::TTFDense1DInput<float> j = i;

        NTFModel::TTFDynamicInputNode node({{"input_1", &i}, {"input_2", &j}});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT(pred.size() == testSize);
        for (size_t i = 0; i < testSize; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], 2., 0.00001);
        }

        NTFModel::TTFDense1DInput<float> iOne(samples, samples.size());
        NTFModel::TTFDense1DInput<float> jOne = iOne;

        NTFModel::TTFDynamicInputNode newNode(2);
        newNode.Assign({{"input_1", &iOne}, {"input_2", &jOne}});
        output = model.Run(&newNode);
        pred = output.Data;
        UNIT_ASSERT(pred.size() == testSize);
        for (size_t i = 0; i < testSize; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], 2., 0.00001);
        }
    }

    Y_UNIT_TEST(LongOutput) {
        ui32 dim = 2;
        NTFModel::TTFDynamicApplier model("x5.pb", dim, "network_output");
        size_t testSize = 2000;
        size_t bs = 128;
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < testSize; ++i) {
            TVector<float> sample(2, i);
            samples.push_back(sample);
        }

        NTFModel::TTFDense1DInput<float> simpleInput(samples, bs);
        NTFModel::TTFDynamicInputNode node({{"main_input", &simpleInput}});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT(pred.size() == testSize);
        for (size_t i = 0; i < bs; ++i) {
            UNIT_ASSERT(pred[i].size() == dim);
            for (size_t j = 0; j < dim; ++j) {
                UNIT_ASSERT_DOUBLES_EQUAL(pred[i][j], 5 * i, 0.00001);
            }
        }
    }

    template <class T>
    typename NTFModel::TTFSparse1DInput<T>::TSparseRow GetSimpleInput(ui32 dim, ui32 n = 100) {
        typename NTFModel::TTFSparse1DInput<T>::TSparseRow row;
        THashSet<NTFModel::TTFInt> indicesSet;
        for (size_t i = 0; i < n; ++i) { // get <99 randint(0, dim) without repetitions
            NTFModel::TTFInt newIndex = RandomNumber<ui32>(dim);
            indicesSet.insert(newIndex);
        }
        row.ColumnIndices = TVector<NTFModel::TTFInt>(indicesSet.begin(), indicesSet.end());
        row.Data = TVector<T>(row.ColumnIndices.size(), T(1));
        return row;
    }

    NTFModel::TTFInt GetIndicesIntersection(
            TArrayRef<NTFModel::TTFInt> indicesA,
            TArrayRef<NTFModel::TTFInt> indicesB
    ) {
        THashMultiSet<NTFModel::TTFInt> intersection;
        for (NTFModel::TTFInt i : indicesA) {
            intersection.insert(i);
        }
        for (NTFModel::TTFInt i : indicesB) {
            intersection.insert(i);
        }
        return (intersection.size());
    }

    Y_UNIT_TEST(RunSparseOneOneInput) {
        ui32 dim = 1000;
        NTFModel::TTFDynamicApplier model("sum_sparse.pb", 1, "network_output");
        NTFModel::TTFSparse1DInput<float>::TSparseRow row = GetSimpleInput<float>(dim);
        NTFModel::TTFSparse1DInput<float> sparseInput(dim, row);
        NTFModel::TTFDynamicInputNode node({{"input", &sparseInput}});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT_DOUBLES_EQUAL(pred[0][0], row.ColumnIndices.size(), 0.00001);
    }

    Y_UNIT_TEST(RunSparseOneTwoInputs) {
        ui32 dim = 1000;
        NTFModel::TTFDynamicApplier model("sum_sparse_2_inputs.pb", 1, "network_output");
        NTFModel::TTFSparse1DInput<float>::TSparseRow rowA = GetSimpleInput<float>(dim);
        NTFModel::TTFSparse1DInput<float> sparseInputA(dim, rowA);

        NTFModel::TTFSparse1DInput<float>::TSparseRow rowB = GetSimpleInput<float>(dim, 10);
        NTFModel::TTFSparse1DInput<float> sparseInputB(dim, rowB);

        float groundTruth = GetIndicesIntersection(rowA.ColumnIndices, rowB.ColumnIndices);

        UNIT_ASSERT(sparseInputA.GetNumSamples() == 1);
        NTFModel::TTFDynamicInputNode node({{"input_1", &sparseInputA}, {"input_2", &sparseInputB}});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT_DOUBLES_EQUAL(pred[0][0], groundTruth, 0.00001);
    }

    Y_UNIT_TEST(RunSparseBatchOneInput) {
        ui32 dim = 1000;
        NTFModel::TTFDynamicApplier model("sum_sparse.pb", 1, "network_output");
        NTFModel::TTFSparse1DInput<float>::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<NTFModel::TTFSparse1DInput<float>::TSparseRow> rows = {row, row, row, row};
        NTFModel::TTFSparse1DInput<float> sparseInput(dim, rows, rows.size());
        NTFModel::TTFDynamicInputNode node({{"input", &sparseInput}});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        for (size_t i = 0; i < rows.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], row.ColumnIndices.size(), 0.00001);
        }
    }

    Y_UNIT_TEST(RunSparseBatchTwoInputs) {
        ui32 dim = 1000;
        NTFModel::TTFDynamicApplier model("sum_sparse_2_inputs.pb", 1, "network_output");
        NTFModel::TTFSparse1DInput<float>::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<NTFModel::TTFSparse1DInput<float>::TSparseRow> rows = {row, row, row, row};
        NTFModel::TTFSparse1DInput<float> sparseInputA(dim, rows, rows.size());
        NTFModel::TTFSparse1DInput<float> sparseInputB(dim, rows, rows.size());

        float groundTruth = 2 * row.ColumnIndices.size();

        UNIT_ASSERT(sparseInputA.GetNumSamples() == 4);
        NTFModel::TTFDynamicInputNode node({{"input_1", &sparseInputA}, {"input_2", &sparseInputB}});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        for (size_t i = 0; i < rows.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], groundTruth, 0.00001);
        }
    }

    Y_UNIT_TEST(RunSparseBatchesOneInput) {
        ui32 dim = 1000;
        size_t testSize = 2000;
        size_t bs = 32;
        NTFModel::TTFDynamicApplier model("sum_sparse.pb", 1, "network_output");
        NTFModel::TTFSparse1DInput<float>::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<NTFModel::TTFSparse1DInput<float>::TSparseRow> rows(testSize, row);
        NTFModel::TTFSparse1DInput<float> sparseInput(dim, rows, bs);
        NTFModel::TTFDynamicInputNode node({{"input", &sparseInput}});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT(pred.size() == testSize);
        for (size_t i = 0; i < testSize; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], row.ColumnIndices.size(), 0.00001);
        }
    }

    Y_UNIT_TEST(RunSparseBatchesTwoInputs) {
        ui32 dim = 1000;
        NTFModel::TTFInt testSize = 2000;
        size_t bs = 32;
        NTFModel::TTFDynamicApplier model("sum_sparse_2_inputs.pb", 1, "network_output");
        NTFModel::TTFSparse1DInput<float>::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<NTFModel::TTFSparse1DInput<float>::TSparseRow> rows(testSize, row);
        NTFModel::TTFSparse1DInput<float> sparseInputA(dim, rows, bs);
        NTFModel::TTFSparse1DInput<float> sparseInputB(dim, rows, bs);

        float groundTruth = 2 * row.ColumnIndices.size();

        UNIT_ASSERT(sparseInputA.GetNumSamples() == testSize);
        NTFModel::TTFDynamicInputNode node({{"input_1", &sparseInputA}, {"input_2", &sparseInputB}});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT(NTFModel::TTFInt(pred.size()) == testSize);
        for (NTFModel::TTFInt i = 0; i < testSize; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], groundTruth, 0.00001);
        }
    }
};

Y_UNIT_TEST_SUITE(TTFStaticApplierTestSuite) {
    using TDenseFloatInput = NTFModel::TTFDense1DInput<float>;

    Y_UNIT_TEST(RunSimpleOneOneInputStatic) {
        NTFModel::TTFApplier<TDenseFloatInput> model("sum.pb", 1, "network_output");
        TVector<float> inputVector(2, 1.);
        TDenseFloatInput simpleInput(inputVector);
        NTFModel::TTFInputNode<TDenseFloatInput> node({"input", &simpleInput});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT_DOUBLES_EQUAL(pred[0][0], 2., 0.00001);
    }

    Y_UNIT_TEST(RunSimpleOneOneInputStaticGraphInit) {
        NTFModel::TTFGraphDef graphDef = NTFModel::ReadModelProtobuf("sum.pb");
        NTFModel::TTFApplier<TDenseFloatInput> model(graphDef, 1, "network_output");
        TVector<float> inputVector(2, 1.);
        TDenseFloatInput simpleInput(inputVector);
        NTFModel::TTFInputNode<TDenseFloatInput> node({"input", &simpleInput});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT_DOUBLES_EQUAL(pred[0][0], 2., 0.00001);
    }

    Y_UNIT_TEST(RunSimpleOneOneInputStaticSessionInit) {
        NTFModel::TTFApplier<TDenseFloatInput> model(
            NTFModel::InitSessionWithGraph("sum.pb"),
            1,
            "network_output"
        );
        TVector<float> inputVector(2, 1.);
        TDenseFloatInput simpleInput(inputVector);
        NTFModel::TTFInputNode<TDenseFloatInput> node({"input", &simpleInput});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT_DOUBLES_EQUAL(pred[0][0], 2., 0.00001);
    }

    Y_UNIT_TEST(RunSimpleOneTwoInputs) {
        NTFModel::TTFApplier<TDenseFloatInput, TDenseFloatInput> model("sum_2_inputs.pb", 1, "network_output");
        TDenseFloatInput i(TVector<float>{1.,});
        TDenseFloatInput j = i;

        NTFModel::TTFInputNode<TDenseFloatInput, TDenseFloatInput> node({"input_1", &i}, {"input_2", &j});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT_DOUBLES_EQUAL(pred[0][0], 2., 0.00001);
    }

    Y_UNIT_TEST(RunSimpleBatchOneInput) {
        NTFModel::TTFApplier<TDenseFloatInput> model("sum.pb", 1, "network_output");
        size_t bs = 128;
        TVector<float> sample(2, 1.);
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < bs; ++i) {
            samples.push_back(sample);
        }
        TDenseFloatInput simpleInput(samples, bs);
        NTFModel::TTFInputNode<TDenseFloatInput> node({"input", &simpleInput});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        for (size_t i = 0; i < bs; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], 2., 0.00001);
        }
    }

    Y_UNIT_TEST(RunSimpleBatchTwoInputs) {
        NTFModel::TTFApplier<TDenseFloatInput, TDenseFloatInput> model("sum_2_inputs.pb", 1, "network_output");
        size_t bs = 128;
        TVector<float> sample(1, 1.);
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < bs; ++i) {
            samples.push_back(sample);
        }

        TDenseFloatInput i(samples, bs);
        TDenseFloatInput j = i;
        NTFModel::TTFInputNode<TDenseFloatInput, TDenseFloatInput> node({"input_1", &i}, {"input_2", &j});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        for (size_t i = 0; i < bs; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], 2., 0.00001);
        }
    }

    Y_UNIT_TEST(RunSimpleBatchesOneInput) {
        NTFModel::TTFApplier<TDenseFloatInput> model("sum.pb", 1, "network_output");
        size_t testSize = 2000;
        size_t bs = 128;
        TVector<float> sample(2, 1.);
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < testSize; ++i) {
            samples.push_back(sample);
        }
        TDenseFloatInput simpleInput(samples, bs);
        NTFModel::TTFInputNode<TDenseFloatInput> node({"input", &simpleInput});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        for (size_t i = 0; i < testSize; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], 2., 0.00001);
        }
    }

    Y_UNIT_TEST(RunSimpleBatchesTwoInputs) {
        size_t testSize = 2000;
        NTFModel::TTFApplier<TDenseFloatInput, TDenseFloatInput> model("sum_2_inputs.pb", 1, "network_output");
        size_t bs = 128;
        TVector<float> sample(1, 1.);
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < testSize; ++i) {
            samples.push_back(sample);
        }

        TDenseFloatInput i(samples, bs);
        TDenseFloatInput j = i;

        NTFModel::TTFInputNode<TDenseFloatInput, TDenseFloatInput> node({"input_1", &i}, {"input_2", &j});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT(pred.size() == testSize);
        for (size_t i = 0; i < testSize; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], 2., 0.00001);
        }

        TDenseFloatInput iOne(samples, samples.size());
        TDenseFloatInput jOne = iOne;

        NTFModel::TTFInputNode<TDenseFloatInput, TDenseFloatInput> newNode;
        newNode.Assign({"input_1", &iOne}, {"input_2", &jOne});
        output = model.Run(&newNode);
        pred = output.Data;
        UNIT_ASSERT(pred.size() == testSize);
        for (size_t i = 0; i < testSize; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], 2., 0.00001);
        }
    }

    Y_UNIT_TEST(LongOutput) {
        ui32 dim = 2;
        NTFModel::TTFApplier<TDenseFloatInput> model("x5.pb", dim, "network_output");
        size_t testSize = 2000;
        size_t bs = 128;
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < testSize; ++i) {
            TVector<float> sample(2, i);
            samples.push_back(sample);
        }

        TDenseFloatInput simpleInput(samples, bs);
        NTFModel::TTFInputNode<TDenseFloatInput> node({"main_input", &simpleInput});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT(pred.size() == testSize);
        for (size_t i = 0; i < bs; ++i) {
            UNIT_ASSERT(pred[i].size() == dim);
            for (size_t j = 0; j < dim; ++j) {
                UNIT_ASSERT_DOUBLES_EQUAL(pred[i][j], 5 * i, 0.00001);
            }
        }
    }

    template <class T>
    typename NTFModel::TTFSparse1DInput<T>::TSparseRow GetSimpleInput(ui32 dim, ui32 n = 100) {
        typename NTFModel::TTFSparse1DInput<T>::TSparseRow row;
        THashSet<NTFModel::TTFInt> indicesSet;
        for (size_t i = 0; i < n; ++i) { // get <99 randint(0, dim) without repetitions
            NTFModel::TTFInt newIndex = RandomNumber<ui32>(dim);
            indicesSet.insert(newIndex);
        }
        row.ColumnIndices = TVector<NTFModel::TTFInt>(indicesSet.begin(), indicesSet.end());
        row.Data = TVector<T>(row.ColumnIndices.size(), T(1));
        return row;
    }

    NTFModel::TTFInt GetIndicesIntersection(TArrayRef<NTFModel::TTFInt> indicesA,
                                            TArrayRef<NTFModel::TTFInt> indicesB) {
        THashMultiSet<NTFModel::TTFInt> intersection;
        for (NTFModel::TTFInt i : indicesA) {
            intersection.insert(i);
        }
        for (NTFModel::TTFInt i : indicesB) {
            intersection.insert(i);
        }
        return (intersection.size());
    }

    using TSparseFloatInput = NTFModel::TTFSparse1DInput<float>;

    Y_UNIT_TEST(RunSparseOneOneInput) {
        ui32 dim = 1000;
        NTFModel::TTFApplier<TSparseFloatInput> model("sum_sparse.pb", 1, "network_output");
        TSparseFloatInput::TSparseRow row = GetSimpleInput<float>(dim);
        TSparseFloatInput sparseInput(dim, row);
        NTFModel::TTFInputNode<TSparseFloatInput> node({"input", &sparseInput});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT_DOUBLES_EQUAL(pred[0][0], row.ColumnIndices.size(), 0.00001);
    }

    Y_UNIT_TEST(RunSparseOneTwoInputs) {
        ui32 dim = 1000;
        NTFModel::TTFApplier<TSparseFloatInput, TSparseFloatInput> model("sum_sparse_2_inputs.pb", 1, "network_output");
        TSparseFloatInput::TSparseRow rowA = GetSimpleInput<float>(dim);
        TSparseFloatInput sparseInputA(dim, rowA);

        TSparseFloatInput::TSparseRow rowB = GetSimpleInput<float>(dim, 10);
        TSparseFloatInput sparseInputB(dim, rowB);

        float groundTruth = GetIndicesIntersection(rowA.ColumnIndices, rowB.ColumnIndices);

        UNIT_ASSERT(sparseInputA.GetNumSamples() == 1);
        NTFModel::TTFInputNode<TSparseFloatInput, TSparseFloatInput> node(
            {"input_1", &sparseInputA}, {"input_2", &sparseInputB}
        );
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT_DOUBLES_EQUAL(pred[0][0], groundTruth, 0.00001);
    }

    Y_UNIT_TEST(RunSparseBatchOneInput) {
        ui32 dim = 1000;
        NTFModel::TTFApplier<TSparseFloatInput> model("sum_sparse.pb", 1, "network_output");
        TSparseFloatInput::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<TSparseFloatInput::TSparseRow> rows = {row, row, row, row};
        TSparseFloatInput sparseInput(dim, rows, rows.size());
        NTFModel::TTFInputNode<TSparseFloatInput> node({"input", &sparseInput});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        for (size_t i = 0; i < rows.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], row.ColumnIndices.size(), 0.00001);
        }
    }

    Y_UNIT_TEST(RunSparseBatchTwoInputs) {
        ui32 dim = 1000;
        NTFModel::TTFApplier<TSparseFloatInput, TSparseFloatInput> model("sum_sparse_2_inputs.pb", 1, "network_output");
        TSparseFloatInput::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<TSparseFloatInput::TSparseRow> rows = {row, row, row, row};
        TSparseFloatInput sparseInputA(dim, rows, rows.size());
        TSparseFloatInput sparseInputB(dim, rows, rows.size());

        float groundTruth = 2 * row.ColumnIndices.size();

        UNIT_ASSERT(sparseInputA.GetNumSamples() == 4);
        NTFModel::TTFInputNode<TSparseFloatInput, TSparseFloatInput> node(
            {"input_1", &sparseInputA}, {"input_2", &sparseInputB}
        );
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        for (size_t i = 0; i < rows.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], groundTruth, 0.00001);
        }
    }

    Y_UNIT_TEST(RunSparseBatchesOneInput) {
        ui32 dim = 1000;
        size_t testSize = 2000;
        size_t bs = 32;
        NTFModel::TTFApplier<TSparseFloatInput> model("sum_sparse.pb", 1, "network_output");
        TSparseFloatInput::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<TSparseFloatInput::TSparseRow> rows(testSize, row);
        TSparseFloatInput sparseInput(dim, rows, bs);
        NTFModel::TTFInputNode<TSparseFloatInput> node({"input", &sparseInput});
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT(pred.size() == testSize);
        for (size_t i = 0; i < testSize; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], row.ColumnIndices.size(), 0.00001);
        }
    }

    Y_UNIT_TEST(RunSparseBatchesTwoInputs) {
        ui32 dim = 1000;
        NTFModel::TTFInt testSize = 2000;
        size_t bs = 32;
        NTFModel::TTFApplier<TSparseFloatInput, TSparseFloatInput> model("sum_sparse_2_inputs.pb", 1, "network_output");
        TSparseFloatInput::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<TSparseFloatInput::TSparseRow> rows(testSize, row);
        TSparseFloatInput sparseInputA(dim, rows, bs);
        TSparseFloatInput sparseInputB(dim, rows, bs);

        float groundTruth = 2 * row.ColumnIndices.size();

        UNIT_ASSERT(sparseInputA.GetNumSamples() == testSize);
        NTFModel::TTFInputNode<TSparseFloatInput, TSparseFloatInput> node(
            {"input_1", &sparseInputA}, {"input_2", &sparseInputB}
        );
        auto output = model.Run(&node);
        TVector<TArrayRef<float>> pred = output.Data;
        UNIT_ASSERT(NTFModel::TTFInt(pred.size()) == testSize);
        for (NTFModel::TTFInt i = 0; i < testSize; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(pred[i][0], groundTruth, 0.00001);
        }
    }
};

Y_UNIT_TEST_SUITE(TTFVanillaApplierTestSuite) {
    Y_UNIT_TEST(RunVanillaSimpleOneOneInput) {
        NTFModel::TTFVanillaApplier model("sum.pb", {"network_output",});
        TVector<float> inputVector(2, 1.);
        NTFModel::TTFDense1DInput<float> simpleInput(inputVector);
        TVector<NTFModel::pair<TString, NTFModel::TTFTensor>> inputs;
        inputs.push_back({"input", simpleInput.GetBatch(0).Tensors[0]});

        TVector<NTFModel::TTFTensor> pred = model.Run(inputs);
        UNIT_ASSERT(pred.size() == 1);
        UNIT_ASSERT(pred[0].dim_size(0) == 1);
        UNIT_ASSERT_DOUBLES_EQUAL(*pred[0].flat<float>().data(), 2., 0.00001);
    }

    Y_UNIT_TEST(RunVanillaEmptyInput) {
        NTFModel::TTFVanillaApplier applier("const_square.pb", {"network_output",});
        TVector<NTFModel::pair<TString, NTFModel::TTFTensor>> inputs;
        auto out = applier.Run(inputs);
        UNIT_ASSERT(out.size() == 1);
        UNIT_ASSERT(out[0].dim_size(0) == 1);
        UNIT_ASSERT_DOUBLES_EQUAL(*out[0].flat<float>().data(), 100., 0.00001);
    }
};
