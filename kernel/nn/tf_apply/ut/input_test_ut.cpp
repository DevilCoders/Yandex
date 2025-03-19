#include <library/cpp/testing/unittest/registar.h>
#include <kernel/nn/tf_apply/inputs.h>
#include <util/random/random.h>

Y_UNIT_TEST_SUITE(TTFDense1DInputTestSuite) {
    TString inputName = "input";

    Y_UNIT_TEST(EmptyStart) {
        NTFModel::TTFDense1DInput<float> inputStatic(2);
        UNIT_ASSERT(inputStatic.GetNumSamples() == 0);
        UNIT_ASSERT(inputStatic.GetNumBatches() == 0);
        auto sampleShape = inputStatic.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == 2);
    }

    Y_UNIT_TEST(AnotherDtype) {
        NTFModel::TTFDense1DInput<NTFModel::TTFInt> inputStatic(30000);
        auto sampleShape = inputStatic.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == 30000);
    }

    Y_UNIT_TEST(AppendSample) {
        NTFModel::TTFDense1DInput<NTFModel::TTFInt> inputStatic(2);
        inputStatic.Append(TVector<NTFModel::TTFInt>{1, 1});
        UNIT_ASSERT(inputStatic.GetNumBatches() == 1);
        UNIT_ASSERT(inputStatic.GetNumSamples() == 1);
    }

    Y_UNIT_TEST(CorrectInitFromSample) {
        TVector<float> sample(100, 0.);
        NTFModel::TTFDense1DInput<float> input(sample);
        UNIT_ASSERT(input.GetNumSamples() == 1);
        UNIT_ASSERT(input.GetNumBatches() == 1);
        auto sampleShape = input.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == 100);

        TVector<int> sampleInt(100, 1);
        NTFModel::TTFDense1DInput<int> inputInt(sampleInt);
        UNIT_ASSERT(inputInt.GetNumSamples() == 1);
        UNIT_ASSERT(inputInt.GetNumBatches() == 1);
        sampleShape = inputInt.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == 100);
        NTFModel::TBatch batch = inputInt.GetBatch(0);
        NTFModel::TTFTensor tensor = batch.Tensors[0];

        UNIT_ASSERT(batch.NumSamples == 1);
        UNIT_ASSERT(tensor.dtype() == NTFModel::ETFDataType::DT_INT32);
    }

    Y_UNIT_TEST(CorrectInitFromSamples) {
        TVector<float> sample(100, 1.);
        TVector<TVector<float>> samples = {sample, sample, sample, sample};
        NTFModel::TTFDense1DInput<float> input(samples, samples.size());
        UNIT_ASSERT(input.GetNumSamples() == 4);
        UNIT_ASSERT(input.GetNumBatches() == 1);
        auto sampleShape = input.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == 100);

        NTFModel::TTFDense1DInput<float> inputBatches(samples, 2);
        UNIT_ASSERT(inputBatches.GetNumSamples() == 4);
        UNIT_ASSERT(inputBatches.GetNumBatches() == 2);
        sampleShape = inputBatches.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == 100);

        TVector<int> sampleInt(100, 1);
        TVector<TVector<int>> samplesInt = {sampleInt, sampleInt, sampleInt};
        NTFModel::TTFDense1DInput<int> inputInt(samplesInt, 1);
        UNIT_ASSERT(inputInt.GetNumSamples() == 3);
        UNIT_ASSERT(inputInt.GetNumBatches() == 3);
        sampleShape = inputInt.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == 100);
    }

    Y_UNIT_TEST(CorrectInputNames) {
        NTFModel::TTFDense1DInput<float> input(100);
        TVector<TString> out = input.GetInputNames(inputName);
        UNIT_ASSERT(out.size() == 1);
        UNIT_ASSERT(out[0] == inputName);
    }

    Y_UNIT_TEST(CorrectTensors) {
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < 10; ++i) {
            float fi = float(i);
            samples.push_back(TVector<float>{fi * 2, fi * 2, fi * 2});
            samples.push_back(TVector<float>{fi / 2, fi / 2, fi / 2});
        }
        samples.push_back(TVector<float>{0.1, 0.1, 0.1});

        NTFModel::TTFDense1DInput<float> input(samples, 2);
        UNIT_ASSERT(input.GetNumBatches() == 11);
        for (size_t i = 0; i < 10; ++i) {
            NTFModel::TBatch batch = input.GetBatch(i);
            UNIT_ASSERT(batch.NumSamples == 2);
            UNIT_ASSERT(batch.Tensors.size() == 1);
            auto tensor = batch.Tensors[0];
            auto eigenTensor = tensor.tensor<float, 2>();
            for (size_t j = 0; j < 3; ++j) {
                UNIT_ASSERT_DOUBLES_EQUAL(eigenTensor(0, j), i * 2, 0.000001);
                UNIT_ASSERT_DOUBLES_EQUAL(eigenTensor(1, j), float(i) / 2, 0.000001);
            }
        }

        NTFModel::TBatch batch = input.GetBatch(10);
        auto tensor = batch.Tensors[0];
        auto eigenTensor = tensor.tensor<float, 2>();
        UNIT_ASSERT(tensor.dim_size(0) == 1);
        for (size_t j = 0; j < 3; ++j) {
            UNIT_ASSERT_DOUBLES_EQUAL(eigenTensor(0, j), 0.1, 0.000001);
        }

        UNIT_ASSERT_EXCEPTION_CONTAINS(
            input.GetBatch(11),
            yexception,
            "batch index 11 out of range [0;11)");
    }

    Y_UNIT_TEST(CorrectAppends) {
        TVector<float> sample(100, 1.);
        TVector<TVector<float>> samples = {sample, sample, sample, sample};
        NTFModel::TTFDense1DInput<float> input(samples, samples.size());
        UNIT_ASSERT(input.GetNumSamples() == 4);
        UNIT_ASSERT(input.GetNumBatches() == 1);

        input.Append(sample);
        UNIT_ASSERT(input.GetNumSamples() == 5);
        UNIT_ASSERT(input.GetNumBatches() == 2);

        input.Append(samples, samples.size());
        UNIT_ASSERT(input.GetNumSamples() == 9);
        UNIT_ASSERT(input.GetNumBatches() == 3);

        input.Append(samples, 3);
        UNIT_ASSERT(input.GetNumSamples() == 13);
        UNIT_ASSERT(input.GetNumBatches() == 5);

        input.AppendBatch(samples.begin() + 1, samples.begin() + 3);
        UNIT_ASSERT(input.GetNumSamples() == 15);
        UNIT_ASSERT(input.GetNumBatches() == 6);
    }

    Y_UNIT_TEST(IncorrectDim) {
        TVector<TVector<float>> samples;
        for (size_t i = 0; i < 10; ++i) {
            float fi = float(i);
            samples.push_back(TVector<float>{fi * 2, fi * 2, fi * 2});
            samples.push_back(TVector<float>{fi / 2, fi / 2, fi / 2});
        }
        samples.push_back(TVector<float>{0.1, 0.1});

        UNIT_ASSERT_EXCEPTION_CONTAINS(
            NTFModel::TTFDense1DInput<float>(samples, 2),
            yexception,
            "expected dim: 3, but given: 2");
    }

    Y_UNIT_TEST(CorrectReset) {
        TVector<float> sample(100, 1.);
        TVector<TVector<float>> samples = {sample, sample, sample, sample};

        NTFModel::TTFDense1DInput<float> input(samples, 2);
        input.Reset();
        UNIT_ASSERT(input.GetNumSamples() == 0);
        UNIT_ASSERT(input.GetNumBatches() == 0);
    }
};

Y_UNIT_TEST_SUITE(TTFSparse1DInputTestSuite) {
    TString inputName = "input";

    template <class T>
    typename NTFModel::TTFSparse1DInput<T>::TSparseRow GetSimpleInput(ui32 dim, ui32 n = 100) {
        typename NTFModel::TTFSparse1DInput<T>::TSparseRow row;
        THashSet<ui32> indicesSet;
        for (size_t i = 0; i < n; ++i) { //get <99 randint(0, dim) without repetitions
            ui32 newIndex = RandomNumber<ui32>(dim);
            indicesSet.insert(newIndex);
        }
        row.ColumnIndices = TVector<NTFModel::TTFInt>(indicesSet.begin(), indicesSet.end());
        row.Data = TVector<T>(row.ColumnIndices.size(), T(1));
        return row;
    }

    Y_UNIT_TEST(EmptyStart) {
        NTFModel::TTFSparse1DInput<float> input(2);
        UNIT_ASSERT(input.GetNumSamples() == 0);
        UNIT_ASSERT(input.GetNumBatches() == 0);
        auto sampleShape = input.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == 2);
    }

    Y_UNIT_TEST(AnotherDtype) {
        NTFModel::TTFSparse1DInput<NTFModel::TTFInt> input(30000);
        auto sampleShape = input.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == 30000);
    }

    Y_UNIT_TEST(AppendSample) {
        ui32 dim = 10000;
        NTFModel::TTFSparse1DInput<float>::TSparseRow row = GetSimpleInput<float>(dim);

        NTFModel::TTFSparse1DInput<float> input(dim);
        input.Append(row);
        UNIT_ASSERT(input.GetNumBatches() == 1);
        UNIT_ASSERT(input.GetNumSamples() == 1);
    }

    Y_UNIT_TEST(CorrectInitFromSample) {
        ui32 dim = 10000;
        NTFModel::TTFSparse1DInput<float>::TSparseRow row = GetSimpleInput<float>(dim);

        NTFModel::TTFSparse1DInput<float> input(dim, row);
        UNIT_ASSERT(input.GetNumSamples() == 1);
        UNIT_ASSERT(input.GetNumBatches() == 1);
        auto sampleShape = input.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == dim);

        NTFModel::TTFSparse1DInput<int>::TSparseRow rowInt;
        rowInt.ColumnIndices = row.ColumnIndices;
        rowInt.Data = TVector<int>(rowInt.ColumnIndices.size(), 1);

        NTFModel::TTFSparse1DInput<int> inputInt(dim, rowInt);
        UNIT_ASSERT(inputInt.GetNumSamples() == 1);
        UNIT_ASSERT(inputInt.GetNumBatches() == 1);
        sampleShape = inputInt.GetSampleShape();
        UNIT_ASSERT(sampleShape.dims() == 1);
        UNIT_ASSERT(sampleShape.dim_size(0) == dim);
    }

    Y_UNIT_TEST(CorrectInitFromSamples) {
        ui32 dim = 10000;
        NTFModel::TTFSparse1DInput<float>::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<NTFModel::TTFSparse1DInput<float>::TSparseRow> rows = {row, row, row, row};

        NTFModel::TTFSparse1DInput<float> input(dim, rows, rows.size());
        UNIT_ASSERT(input.GetNumSamples() == 4);
        UNIT_ASSERT(input.GetNumBatches() == 1);

        NTFModel::TTFSparse1DInput<float> inputBatch(dim, rows, 2);
        UNIT_ASSERT(inputBatch.GetNumSamples() == 4);
        UNIT_ASSERT(inputBatch.GetNumBatches() == 2);

        NTFModel::TTFSparse1DInput<int>::TSparseRow rowInt;
        rowInt.ColumnIndices = row.ColumnIndices;
        rowInt.Data = TVector<int>(rowInt.ColumnIndices.size(), 1);
        TVector<NTFModel::TTFSparse1DInput<int>::TSparseRow> rowsInt = {
            rowInt, rowInt, rowInt, rowInt
        };

        NTFModel::TTFSparse1DInput<int> inputInt(dim, rowsInt, 2);
        UNIT_ASSERT(inputInt.GetNumSamples() == 4);
        UNIT_ASSERT(inputInt.GetNumBatches() == 2);
    }

    Y_UNIT_TEST(CorrectInputNames) {
        ui32 dim = 10000;
        NTFModel::TTFSparse1DInput<float> input(dim);

        TVector<TString> out = input.GetInputNames(inputName);
        UNIT_ASSERT(out.size() == 3);
        UNIT_ASSERT(out[0] == inputName + "/indices");
        UNIT_ASSERT(out[1] == inputName + "/values");
        UNIT_ASSERT(out[2] == inputName + "/shape");
    }

    Y_UNIT_TEST(CorrectTensors) {
        ui32 dim = 10000;
        TVector<NTFModel::TTFSparse1DInput<float>::TSparseRow> rows;
        THashSet<ui32> indicesSet;
        for (size_t i = 0; i < 100; ++i) {
            ui32 newIndex = RandomNumber<ui32>(dim);
            indicesSet.insert(newIndex);
        }
        TVector<NTFModel::TTFInt> nonZeroColumns(indicesSet.begin(), indicesSet.end());

        for (size_t i = 0; i < 10; ++i) {
            float fi = float(i);
            NTFModel::TTFSparse1DInput<float>::TSparseRow row;
            row.Data = TVector<float>(nonZeroColumns.size(), fi * 2);
            row.ColumnIndices = nonZeroColumns;

            rows.push_back(row);
            row.Data = TVector<float>(nonZeroColumns.size(), fi / 2);
            rows.push_back(row);
        }
        NTFModel::TTFSparse1DInput<float>::TSparseRow row;
        row.Data = TVector<float>(nonZeroColumns.size(), 0.1);
        rows.push_back(row);
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            NTFModel::TTFSparse1DInput<float>(dim, rows, 2),
            yexception,
            "given " + ToString(nonZeroColumns.size()) + " values, but 0 column indices");
        rows[rows.size() - 1].ColumnIndices = nonZeroColumns;

        NTFModel::TTFSparse1DInput<float> input(dim, rows, 2);

        UNIT_ASSERT(input.GetNumBatches() == 11);

        for (size_t i = 0; i < 10; ++i) {
            NTFModel::TBatch batch = input.GetBatch(i);
            UNIT_ASSERT(batch.NumSamples == 2);

            TVector<NTFModel::TTFTensor> tensors = batch.Tensors;
            UNIT_ASSERT(tensors.size() == 3);
            NTFModel::TTFTensor vTensor = tensors[1];
            UNIT_ASSERT(ui32(vTensor.dim_size(0)) == nonZeroColumns.size() * 2);
            auto eigenTensor = vTensor.tensor<float, 1>();
            for (size_t j = 0; j < nonZeroColumns.size(); ++j) {
                UNIT_ASSERT_DOUBLES_EQUAL(eigenTensor(j), i * 2, 0.000001);
                UNIT_ASSERT_DOUBLES_EQUAL(eigenTensor(j + nonZeroColumns.size()), float(i) / 2, 0.000001);
            }

            NTFModel::TTFTensor indTensor = tensors[0];
            UNIT_ASSERT(ui32(indTensor.dim_size(0)) == nonZeroColumns.size() * 2);
            UNIT_ASSERT(indTensor.dim_size(1) == 2);
            auto indEigenTensor = indTensor.tensor<NTFModel::TTFInt, 2>();
            for (size_t u = 0; u < nonZeroColumns.size(); ++u) {
                UNIT_ASSERT(indEigenTensor(u, 0) == 0);
                UNIT_ASSERT(indEigenTensor(u, 1) == nonZeroColumns[u]);

                UNIT_ASSERT(indEigenTensor(u + nonZeroColumns.size(), 0) == 1);
                UNIT_ASSERT(indEigenTensor(u + nonZeroColumns.size(), 1) == nonZeroColumns[u]);
            }

            NTFModel::TTFTensor shape = tensors[2];
            auto shTensor = shape.tensor<NTFModel::TTFInt, 1>();
            UNIT_ASSERT(shape.dims() == 1);
            UNIT_ASSERT(shape.dim_size(0) == 2);
            UNIT_ASSERT(shTensor(0) == 2);
            UNIT_ASSERT(shTensor(1) == dim);
        }

        NTFModel::TBatch batch = input.GetBatch(10);
        TVector<NTFModel::TTFTensor> tensors = batch.Tensors;

        NTFModel::TTFTensor vTensor = tensors[1];
        UNIT_ASSERT(ui32(vTensor.dim_size(0)) == nonZeroColumns.size());
        auto eigenTensor = vTensor.tensor<float, 1>();
        for (size_t j = 0; j < nonZeroColumns.size(); ++j) {
            UNIT_ASSERT_DOUBLES_EQUAL(eigenTensor(j), 0.1, 0.000001);
        }

        NTFModel::TTFTensor indTensor = tensors[0];
        UNIT_ASSERT(ui32(indTensor.dim_size(0)) == nonZeroColumns.size());
        UNIT_ASSERT(indTensor.dim_size(1) == 2);
        auto indEigenTensor = indTensor.tensor<NTFModel::TTFInt, 2>();
        for (size_t u = 0; u < nonZeroColumns.size(); ++u) {
            UNIT_ASSERT(indEigenTensor(u, 0) == 0);
            UNIT_ASSERT(indEigenTensor(u, 1) == nonZeroColumns[u]);
        }

        NTFModel::TTFTensor shape = tensors[2];
        auto shTensor = shape.tensor<NTFModel::TTFInt, 1>();
        UNIT_ASSERT(shape.dims() == 1);
        UNIT_ASSERT(shape.dim_size(0) == 2);
        UNIT_ASSERT(shTensor(0) == 1);
        UNIT_ASSERT(shTensor(1) == dim);
    }

    Y_UNIT_TEST(CorrectAppends) {
        ui32 dim = 10000;
        NTFModel::TTFSparse1DInput<float>::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<NTFModel::TTFSparse1DInput<float>::TSparseRow> rows = {row, row, row, row};

        NTFModel::TTFSparse1DInput<float> input(dim, rows, rows.size());
        input.Append(row);
        UNIT_ASSERT(input.GetNumSamples() == 5);
        UNIT_ASSERT(input.GetNumBatches() == 2);

        input.Append(rows, rows.size());
        UNIT_ASSERT(input.GetNumSamples() == 9);
        UNIT_ASSERT(input.GetNumBatches() == 3);

        input.Append(rows, 3);
        UNIT_ASSERT(input.GetNumSamples() == 13);
        UNIT_ASSERT(input.GetNumBatches() == 5);

        input.AppendBatch(rows.begin() + 1, rows.begin() + 3);
        UNIT_ASSERT(input.GetNumSamples() == 15);
        UNIT_ASSERT(input.GetNumBatches() == 6);
    }

    Y_UNIT_TEST(CorrectReset) {
        ui32 dim = 10000;
        NTFModel::TTFSparse1DInput<float>::TSparseRow row = GetSimpleInput<float>(dim);
        TVector<NTFModel::TTFSparse1DInput<float>::TSparseRow> rows = {row, row, row, row};

        NTFModel::TTFSparse1DInput<float> input(dim, rows, rows.size());
        input.Reset();
        UNIT_ASSERT(input.GetNumSamples() == 0);
        UNIT_ASSERT(input.GetNumBatches() == 0);
    }
};

Y_UNIT_TEST_SUITE(TTFInputNodeTestSuite) {
    TString inputName = "input";

    Y_UNIT_TEST(TTFDynamicInputNode) {
        TVector<float> sample(100, 1.);
        TVector<TVector<float>> samples = {sample, sample, sample, sample};
        NTFModel::TTFDense1DInput<float> inputA(samples, samples.size());
        NTFModel::TTFDense1DInput<float> inputB(samples, samples.size());
        NTFModel::TTFDynamicInputNode node(2);
        UNIT_ASSERT_EXCEPTION(node.Valid(), yexception);
        node.Assign({
            {inputName, &inputA},
            {inputName, &inputB}
        });
        UNIT_ASSERT(node.Valid());
        UNIT_ASSERT(node.GetNumBatches() == 1);

        UNIT_ASSERT_EXCEPTION_CONTAINS(
            node.Set(0, {inputName, &inputA}),
            yexception,
            "trying to replace non-nullptr input at index 0"
        );
        NTFModel::TTFDynamicInputNode newNode(2);
        newNode.Set(0, {inputName, &inputA});
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            newNode.Get(),
            yexception,
            "pointer to input 1 is nullptr"
        );
        newNode.Set(1, {inputName, &inputB});
        UNIT_ASSERT(newNode.Valid());
        UNIT_ASSERT(newNode.GetNumBatches() == 1);
        inputA.Append(sample);
    }

    Y_UNIT_TEST(TTFStaticInputNode) {
        TVector<float> sample(100, 1.);
        TVector<TVector<float>> samples = {sample, sample, sample, sample};
        NTFModel::TTFDense1DInput<float> inputA(samples, samples.size());
        NTFModel::TTFDense1DInput<float> inputB(samples, samples.size());
        NTFModel::TTFInputNode<
            NTFModel::TTFDense1DInput<float>,
            NTFModel::TTFDense1DInput<float>
        > node;
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            node.Valid(),
            yexception,
            "pointer to input 0 is nullptr" // another exception because of calling TTFInputNode ctor
        );
        node.Assign(
            std::make_pair(inputName + "_static", &inputA),
            std::make_pair(inputName + "_static2", &inputB)
        );
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            node.Set<0>({inputName, &inputA}),
            yexception,
            "trying to replace non-nullptr input at index 0"
        );

        UNIT_ASSERT(node.Valid());
        UNIT_ASSERT(node.GetNumBatches() == 1);

        NTFModel::TTFInputNode<
            NTFModel::TTFDense1DInput<float>,
            NTFModel::TTFDense1DInput<float>
        > newNode;
        newNode.Set<0>({inputName, &inputA});
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            newNode.Get(),
            yexception,
            "pointer to input 1 is nullptr"
        );
        newNode.Set<1>({inputName, &inputB});
        UNIT_ASSERT(newNode.Valid());
        UNIT_ASSERT(newNode.GetNumBatches() == 1);
        inputA.Append(sample);
    }

    Y_UNIT_TEST(IncorrectAssign) {
        TVector<float> sample(100, 1.);
        NTFModel::TTFDense1DInput<float> input(sample);
        NTFModel::TTFDynamicInputNode node(2);
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            node.Assign({{"a", &input}, {"a", &input}, {"a", &input}}),
            yexception,
            "trying to assign 2 inputs with 3 values"
        );
    }

    Y_UNIT_TEST(GetExceptions) {
        TVector<float> sample(100, 1.);
        NTFModel::TTFDense1DInput<float> input(sample);
        NTFModel::TTFDynamicInputNode node(1);
        UNIT_ASSERT_EXCEPTION(node.Get(), yexception);

        NTFModel::TTFDynamicInputNode node1(2);
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            node1.Get(),
            yexception,
            "pointer to input 0 is nullptr"
        );
        node1.Assign({{"a", &input}, {"a", &input}});
        node1.Get();
        node1.Next();
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            node1.Get(),
            yexception,
            "input 0 has only 1 batches, trying to get batch 1"
        );
        input.Append(sample);
        NTFModel::TTFDense1DInput<float> input2(sample);
        input2.AppendBatch({sample, sample});
        NTFModel::TTFDynamicInputNode node2({{"a", &input}, {"a", &input2}});
        node2.Get();
        node2.Next();
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            node2.Get(),
            yexception,
            "inconsistent size of batch: input 0 has 1 samples in batch, while input 1 has 2 samples in batch"
        );
    }
};
