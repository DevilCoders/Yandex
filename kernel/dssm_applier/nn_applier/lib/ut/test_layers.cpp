#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/cast.h>
#include <util/generic/xrange.h>
#include <util/generic/ymath.h>
#include <util/random/mersenne.h>

using namespace NNeuralNetApplier;

namespace {

    void FillMatrixRandom(TMatrix* matrix, TMersenne<ui64>& rng) {
        for (float& x : matrix->AsFlatArray()) {
            x = rng.GenRandReal4() * 2.0 - 1.0;
        }
    }

    TVector<float> GetUniformBucketValues(float minValue, float maxValue, ui32 numBits) {
        float coef = (maxValue - minValue) / ((1 << numBits) - 1);
        TVector<float> result;
        for (size_t cur = 0; cur < (1 << numBits); ++cur) {
            result.push_back(cur * coef + minValue);
        }
        return result;
    }

    template <class TFunc>
    void TestElementwiseTransform() {
        TMatrix* X = new TMatrix();
        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(X);

        TElementwiseTransform<TFunc> reluLayer("input", "output");

        reluLayer.Init(context);

        X->Resize(2, 5);
        TVector<float> x = {-1.0, -0.5, 0.0, 0.5, 1.0};
        for (size_t i = 0; i < 5; ++i) {
            (*X)[0][i] = x[i];
            (*X)[1][i] = x[i];
        }

        reluLayer.Apply(inputContext);

        TMatrix* result = (TMatrix*)inputContext["output"].Get();

        UNIT_ASSERT_VALUES_EQUAL(2, result->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(5, result->GetNumColumns());

        for (size_t i = 0; i < result->GetNumRows(); ++i) {
            for (size_t j = 0; j < result->GetNumColumns(); ++j) {
                float expected = TFunc::Fprop(x[j]);
                UNIT_ASSERT_VALUES_EQUAL(expected, (*result)[0][j]);
            }
        }
    }

} // unnamed namespace

Y_UNIT_TEST_SUITE(TestLayers) {
    Y_UNIT_TEST(TestRluTransform) {
        TestElementwiseTransform<TRlu>();
    }

    Y_UNIT_TEST(TestTanhLayer) {
        TestElementwiseTransform<TTanh>();
    }

    Y_UNIT_TEST(TestEluLayer) {
        TestElementwiseTransform<TElu>();
    }

    Y_UNIT_TEST(TestSeluLayer) {
        TestElementwiseTransform<TSelu>();
    }

    Y_UNIT_TEST(TestSigmoidLayer) {
        TestElementwiseTransform<TSigmoid>();
    }

    Y_UNIT_TEST(TestLinearLayer) {
        TestElementwiseTransform<TLinear>();
    }

    Y_UNIT_TEST(TestSoftplusLayer) {
        TestElementwiseTransform<TSoftplus>();
    }

    Y_UNIT_TEST(TestRlu) {
        for (double val: {-1.5, 1.0, -0.5, 0.0, 0.5, 1.0, 1.5, 12.4}) {
            UNIT_ASSERT_DOUBLES_EQUAL(val * (val > 0), TRlu::Fprop(val), 1e-5);
        }
        UNIT_ASSERT(TRlu::GetName() == "rlu");
    }

    Y_UNIT_TEST(TestElu) {
        for (double val: {-1.5, 1.0, -0.5, 0.0, 0.5, 1.0, 1.5, 12.4}) {
            UNIT_ASSERT_DOUBLES_EQUAL((val >= 0 ? val: exp(val) - 1), TElu::Fprop(val), 1e-5);
        }
        UNIT_ASSERT(TElu::GetName() == "elu");
    }

    Y_UNIT_TEST(TestSelu) {
        const float ALPHA = 1.673;
        const float SCALE = 1.051;
        for (double val: {-1.5, 1.0, -0.5, 0.0, 0.5, 1.0, 1.5, 12.4}) {
            UNIT_ASSERT_DOUBLES_EQUAL((val >= 0 ? SCALE * val: SCALE * (ALPHA * exp(val) - ALPHA)), TSelu::Fprop(val), 1e-5);
        }
        UNIT_ASSERT(TSelu::GetName() == "selu");
    }

    Y_UNIT_TEST(TestTanh) {
        for (double val: {-1.5, 1.0, -0.5, 0.0, 0.5, 1.0, 1.5, 12.4}) {
            UNIT_ASSERT_DOUBLES_EQUAL(tanh(val), TTanh::Fprop(val), 1e-5);
        }
        UNIT_ASSERT(TTanh::GetName() == "tanh");
    }

    Y_UNIT_TEST(TestSigmoid) {
        for (double val: {-1.5, 1.0, -0.5, 0.0, 0.5, 1.0, 1.5, 12.4}) {
            UNIT_ASSERT_DOUBLES_EQUAL(1.0 / (1.0 + exp(-val)), TSigmoid::Fprop(val), 1e-5);
        }
        UNIT_ASSERT(TSigmoid::GetName() == "sigmoid");
    }

    Y_UNIT_TEST(TestLinear) {
        for (double val: {-1.5, 1.0, -0.5, 0.0, 0.5, 1.0, 1.5, 12.4}) {
            UNIT_ASSERT_DOUBLES_EQUAL(val, TLinear::Fprop(val), 1e-5);
        }
        UNIT_ASSERT(TLinear::GetName() == "linear");
    }

    Y_UNIT_TEST(TesSoftplus) {
        for (double val: {-1.5, 1.0, -0.5, 0.0, 0.5, 1.0, 1.5, 12.4}) {
            UNIT_ASSERT_DOUBLES_EQUAL(log(1 + exp(static_cast<double>(val))), TSoftplus::Fprop(val), 1e-5);
        }
        UNIT_ASSERT(TSoftplus::GetName() == "softplus");
    }

    Y_UNIT_TEST(TColumnElemwiseTransformModule) {
        TMatrix* X = new TMatrix();
        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(X);

        TColumnElemwiseTransformLayer layer("input", "output", {EFunction::Sigmoid, EFunction::Elu});

        layer.Init(context);

        X->Resize(3, 2);
        TVector<float> x = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 0.3f};
        for (auto rowIdx: xrange(3)) {
            for (auto colIdx: xrange(2)) {
                (*X)[rowIdx][colIdx] = x[colIdx + rowIdx * X->GetNumColumns()];
            }
        }

        layer.Apply(inputContext);

        TMatrix* result = (TMatrix*)inputContext["output"].Get();

        TMatrix expected;
        expected.Resize(3, 2);
        expected[0][0] = TSigmoid::Fprop((*X)[0][0]);
        expected[0][1] = TElu::Fprop((*X)[0][1]);
        expected[1][0] = TSigmoid::Fprop((*X)[1][0]);
        expected[1][1] = TElu::Fprop((*X)[1][1]);
        expected[2][0] = TSigmoid::Fprop((*X)[2][0]);
        expected[2][1] = TElu::Fprop((*X)[2][1]);

        UNIT_ASSERT_VALUES_EQUAL(3, expected.GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, expected.GetNumColumns());

        for (size_t i = 0; i < result->GetNumRows(); ++i) {
            for (size_t j = 0; j < result->GetNumColumns(); ++j) {
                UNIT_ASSERT_DOUBLES_EQUAL(expected[i][j], (*result)[i][j], 1e-5);
            }
        }
    }

    Y_UNIT_TEST(TestBitVector) {
        TMersenne<ui64> rng(15413);
        // fixed number of bits
        for (size_t numBits = 1; numBits <= 8; ++numBits) {
            TVector<size_t> vals(120);
            size_t numChars = numBits * 15;
            TVector<ui8> data(numChars);
            size_t mask = (1 << numBits) - 1;
            {
                TBitVectorWriter v(data.data());
                for (auto i : xrange(vals.size())) {
                    vals[i] = rng.GenRand();
                    v.Write(vals[i], numBits);
                }
                UNIT_ASSERT(v.GetData() == data.data() + numChars - 1);
            }
            TBitVectorReader v(data.data());
            for (auto i : xrange(vals.size())) {
                size_t val = v.Get(numBits);
                UNIT_ASSERT(val == (vals[i] & mask));
            }
            UNIT_ASSERT(v.GetData() == data.data() + numChars - 1);
        }

        // different number of bits
        TVector<size_t> vals(120);
        size_t numChars = (36 * 15) / 8 + 1;
        TVector<ui8> data(numChars);
        {
            TBitVectorWriter v(data.data());
            for (auto i : xrange(vals.size())) {
                size_t numBits = i % 8 + 1;
                vals[i] = rng.GenRand();
                v.Write(vals[i], numBits);
            }
            UNIT_ASSERT(v.GetData() == data.data() + numChars - 1);
        }
        TBitVectorReader v(data.data());
        for (auto i : xrange(vals.size())) {
            size_t numBits = i % 8 + 1;
            size_t mask = (1 << numBits) - 1;
            size_t val = v.Get(numBits);
            UNIT_ASSERT(val == (vals[i] & mask));
        }
        UNIT_ASSERT(v.GetData() == data.data() + numChars - 1);
    }

    Y_UNIT_TEST(TestCompressorDecompressor) {
        for (size_t numBits : {0, 5, 6, 7, 8}) {
            TMatrix* X = new TMatrix();
            TContext context;
            TEvalContext inputContext(&context);
            inputContext["input"].Reset(X);
            TCompressorLayer compressor("input", "compressed", 32, -2, 13.5, numBits);
            TDecompressorLayer decompressor("compressed", "output", 32, -2, 13.5, numBits);
            compressor.Init(context);
            decompressor.Init(context);
            X->Resize(2, 5);
            TVector<float> x = { -2.5, -1.5, 3.0, 0.5, 14 };
            TVector<float> y = { -2.6f, -1.2f, 3.4f, 0.1f, 13.9f };
            for (size_t i = 0; i < 5; ++i) {
                (*X)[0][i] = x[i];
                (*X)[1][i] = y[i];
            }
            compressor.Apply(inputContext);
            decompressor.Apply(inputContext);

            TMatrix* result = (TMatrix*)inputContext["output"].Get();
            TCharMatrix* compressed = (TCharMatrix*)inputContext["compressed"].Get();
            if (numBits <= 5) {
                UNIT_ASSERT_VALUES_EQUAL(compressed->GetNumRows(), 2);
                UNIT_ASSERT_VALUES_EQUAL(compressed->GetNumColumns(), 4);
                UNIT_ASSERT_VALUES_EQUAL(compressed->GetNumUnpackedColumns(), 5);
            }

            for (size_t i = 0; i < result->GetNumRows(); ++i) {
                for (size_t j = 0; j < result->GetNumColumns(); ++j) {
                    float val = round((*X)[i][j] * 2) / 2;
                    val = Min(Max(val, -2.0f), 13.5f);
                    UNIT_ASSERT_VALUES_EQUAL(val, (*result)[i][j]);
                }
            }
        }
    }

    Y_UNIT_TEST(TestSoftmaxCompressorDecompressor) {
        TMatrix* X = new TMatrix();
        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(X);
        TOneHotCompressorLayer compressor("input", "compressed", 8);
        TOneHotDecompressorLayer decompressor("compressed", "output", 8);
        compressor.Init(context);
        decompressor.Init(context);
        X->Resize(2, 40);
        TVector<float> x = {1, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 1,
            0, 0, 0, 0, 0, 1, 0, 0,
            0, 0, 0, 1, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0, 0};
        TVector<float> y = { 0, 0, 0, 0, 0, 0, 1, 0,
            0, 0, 0, 0, 1, 0, 0, 0,
            0, 0, 0, 0, 1, 0, 0, 0,
            0, 0, 0, 1, 0, 0, 0, 0,
            0, 0, 1, 0, 0, 0, 0, 0 };
        for (size_t i = 0; i < x.size(); ++i) {
            (*X)[0][i] = x[i];
            (*X)[1][i] = y[i];
        }
        compressor.Apply(inputContext);
        decompressor.Apply(inputContext);

        TMatrix* result = (TMatrix*)inputContext["output"].Get();
        TCharMatrix* compressed = (TCharMatrix*)inputContext["compressed"].Get();
        UNIT_ASSERT_VALUES_EQUAL(compressed->GetNumRows(), 2);
        UNIT_ASSERT_VALUES_EQUAL(compressed->GetNumColumns(), 2);
        UNIT_ASSERT_VALUES_EQUAL(compressed->GetNumUnpackedColumns(), 40);

        for (size_t i = 0; i < result->GetNumRows(); ++i) {
            for (size_t j = 0; j < result->GetNumColumns(); ++j) {
                UNIT_ASSERT_VALUES_EQUAL((*X)[i][j], (*result)[i][j]);
            }
        }
    }

    Y_UNIT_TEST(TestTFieldExtractorLayer) {
        TSamplesVector* input = new TSamplesVector();
        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);

        THashMap<TString, TString> annotationToOutput;
        annotationToOutput["query"] = "queryVariable";
        annotationToOutput["title"] = "titleVariable";

        TFieldExtractorLayer fieldExtractorLayer("input", annotationToOutput);

        fieldExtractorLayer.Init(context);

        TVector<TAtomicSharedPtr<ISample>> samples;
        samples.resize(2);
        samples[0].Reset(new TSample(
            {"query", "title"},
            {"first query", "first title"}));
        samples[1].Reset(new TSample(
            {"query", "title"},
            {"second query", "second title"}));
        input->SetSamples(samples);

        fieldExtractorLayer.Apply(inputContext);

        TTextsVector* queries = (TTextsVector*)inputContext.at("queryVariable").Get();
        TTextsVector* titles = (TTextsVector*)inputContext.at("titleVariable").Get();

        TVector<TString> expectedQueries = {"first query", "second query"};
        TVector<TString> expectedTitles = {"first title", "second title"};
        UNIT_ASSERT_VALUES_EQUAL(expectedQueries, queries->Texts);
        UNIT_ASSERT_VALUES_EQUAL(expectedTitles, titles->Texts);

        samples[1].Reset(new TSample(
            {"query", "title", "url"},
            {"second query", "second title", "second url"}));
        input->SetSamples(samples);
        fieldExtractorLayer.Apply(inputContext);
        UNIT_ASSERT_VALUES_EQUAL(expectedQueries, queries->Texts);
        UNIT_ASSERT_VALUES_EQUAL(expectedTitles, titles->Texts);

        samples[1].Reset(new TSample(
            {"query_typo", "title"},
            {"second query", "second title"}));
        input->SetSamples(samples);
        UNIT_ASSERT_EXCEPTION(fieldExtractorLayer.Apply(inputContext), yexception);
    }

    Y_UNIT_TEST(TestTStringToSparseMatrixLayer) {
        TTextsVector* titles = new TTextsVector();

        THashMap<TUtf16String, size_t> mapping;
        mapping[u"abc"] = 0;
        mapping[u"bcd"] = 2;
        TTrigramTokenizer* trigramTokenizer = new TTrigramTokenizer(mapping, true);
        TWordTokenizer* wordTokenizer = new TWordTokenizer(mapping, false);

        TSparsifier* sparsifier = new TSparsifier({trigramTokenizer, wordTokenizer});

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["titles"].Reset(titles);
        context["sparsifier"].Reset(sparsifier);

        TStringToSparseMatrixLayer stringsToSparseMatrixLayer(
            "titles", "sparsifier", "output");

        stringsToSparseMatrixLayer.Init(context);

        titles->Texts = {"abcde", "abc", "bcd"};

        stringsToSparseMatrixLayer.Apply(inputContext);

        TSparseMatrix* output = (TSparseMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->Vectors.size());
        UNIT_ASSERT_VALUES_EQUAL(TVector<size_t>({0, 2}), output->Vectors[0].Indexes);
        UNIT_ASSERT_VALUES_EQUAL(TVector<size_t>({0}), output->Vectors[1].Indexes);
        UNIT_ASSERT_VALUES_EQUAL(TVector<size_t>({2}), output->Vectors[2].Indexes);
    }

    Y_UNIT_TEST(TestTVectorParseLayer) {
        TTextsVector* input = new TTextsVector();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);

        input->Texts = {"1.5,-0.3,10,1e-3", "0.1,0.3,0.7,1.0"};
        const ui64 expectedSize = 4;

        TVectorParseLayer layer("input", "output", expectedSize, ',');
        layer.Init(context);
        layer.Apply(inputContext);

        TMatrix* output = dynamic_cast<TMatrix*>(inputContext.at("output").Get());

        UNIT_ASSERT_VALUES_EQUAL(input->Texts.size(), output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(expectedSize, output->GetNumColumns());
        UNIT_ASSERT_VALUES_EQUAL(TVector<float>({1.5f, -0.3f, 10.0f, 0.001f}), TVector<float>((*output)[0], (*output)[0] + expectedSize));
        UNIT_ASSERT_VALUES_EQUAL(TVector<float>({0.1f, 0.3f, 0.7f, 1.0f}), TVector<float>((*output)[1], (*output)[1] + expectedSize));
    }

    Y_UNIT_TEST(TestRepeatedJsonParserLayer) {
        TTextsVector* input = new TTextsVector();

        input->Texts.resize(2);
        input->Texts[0] = "{\"texts\" : [\"abc\", \"bca\"], \"numbers\": [\"123\", \"321\"], \"weight\": [0.5, 0.6]}";
        input->Texts[1] = "{\"texts\" : [\"fdt\", \"qrtad\", \"qrtad2\"], \"numbers\": [\"123\", \"321\", \"535\"], \"weight\": [0.1, 0.5, 0.33]}";

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);

        TRepeatedParserLayer<TJsonParser> layer("input", {"texts", "numbers", "weight"},
            {"texts_ctx", "numbers_ctx", "weight_ctx"}, "groups"
        );
        layer.Init(context);
        layer.Apply(inputContext);

        TGenericMatrix<ui64>* groups = dynamic_cast<TGenericMatrix<ui64>*>(inputContext.at("groups").Get());
        UNIT_ASSERT_VALUES_EQUAL(+groups->GetNumRows(), 1);
        UNIT_ASSERT_VALUES_EQUAL(+groups->GetNumColumns(), 2);
        UNIT_ASSERT_VALUES_EQUAL((*groups)[0][0], 2);
        UNIT_ASSERT_VALUES_EQUAL((*groups)[0][1], 3);

        TTextsVector* texts = dynamic_cast<TTextsVector*>(inputContext.at("texts_ctx").Get());
        UNIT_ASSERT_VALUES_EQUAL(texts->Texts.size(), 5);
        UNIT_ASSERT_EQUAL(texts->Texts[0], "abc");
        UNIT_ASSERT_EQUAL(texts->Texts[1], "bca");
        UNIT_ASSERT_EQUAL(texts->Texts[2], "fdt");
        UNIT_ASSERT_EQUAL(texts->Texts[3], "qrtad");
        UNIT_ASSERT_EQUAL(texts->Texts[4], "qrtad2");

        TTextsVector* numbers = dynamic_cast<TTextsVector*>(inputContext.at("numbers_ctx").Get());
        UNIT_ASSERT_VALUES_EQUAL(numbers->Texts.size(), 5);
        UNIT_ASSERT_EQUAL(numbers->Texts[0], "123");
        UNIT_ASSERT_EQUAL(numbers->Texts[1], "321");
        UNIT_ASSERT_EQUAL(numbers->Texts[2], "123");
        UNIT_ASSERT_EQUAL(numbers->Texts[3], "321");
        UNIT_ASSERT_EQUAL(numbers->Texts[4], "535");

        TTextsVector* weights = dynamic_cast<TTextsVector*>(inputContext.at("weight_ctx").Get());
        UNIT_ASSERT_VALUES_EQUAL(weights->Texts.size(), 5);
        UNIT_ASSERT_EQUAL(weights->Texts[0], "0.5");
        UNIT_ASSERT_EQUAL(weights->Texts[1], "0.6");
        UNIT_ASSERT_EQUAL(weights->Texts[2], "0.1");
        UNIT_ASSERT_EQUAL(weights->Texts[3], "0.5");
        UNIT_ASSERT_EQUAL(weights->Texts[4], "0.33");
    }

    Y_UNIT_TEST(TestRepeatedBinaryParserLayer) {
        TTextsVector* input = new TTextsVector();
        input->Texts.resize(2);

        {
            TRepeatedFieldProto field;
            TRepeatedFieldEntryProto texts;
            TRepeatedFieldEntryProto numbers;
            TRepeatedFieldEntryProto weights;
            texts.SetKey("texts");
            texts.AddValues("abc");
            texts.AddValues("bca");

            numbers.SetKey("numbers");
            numbers.AddValues("123");
            numbers.AddValues("321");

            weights.SetKey("weight");
            weights.AddValues("0.5");
            weights.AddValues("0.6");

            *field.AddEntries() = texts;
            *field.AddEntries() = numbers;
            *field.AddEntries() = weights;
            input->Texts[0] = field.SerializeAsString();
        }

        {
            TRepeatedFieldProto field;
            TRepeatedFieldEntryProto texts;
            TRepeatedFieldEntryProto numbers;
            TRepeatedFieldEntryProto weights;
            texts.SetKey("texts");
            texts.AddValues("fdt");
            texts.AddValues("qrtad");
            texts.AddValues("qrtad2");

            numbers.SetKey("numbers");
            numbers.AddValues("123");
            numbers.AddValues("321");
            numbers.AddValues("535");

            weights.SetKey("weight");
            weights.AddValues("0.1");
            weights.AddValues("0.5");
            weights.AddValues("0.33");

            *field.AddEntries() = texts;
            *field.AddEntries() = numbers;
            *field.AddEntries() = weights;
            input->Texts[1] = field.SerializeAsString();
        }

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);

        TRepeatedParserLayer<TBinaryParser> layer("input", {"texts", "numbers", "weight"},
            {"texts_ctx", "numbers_ctx", "weight_ctx"}, "groups"
        );
        layer.Init(context);
        layer.Apply(inputContext);

        TGenericMatrix<ui64>* groupsJson = dynamic_cast<TGenericMatrix<ui64>*>(inputContext.at("groups").Get());
        UNIT_ASSERT_VALUES_EQUAL(+groupsJson->GetNumRows(), 1);
        UNIT_ASSERT_VALUES_EQUAL(+groupsJson->GetNumColumns(), 2);
        UNIT_ASSERT_VALUES_EQUAL((*groupsJson)[0][0], 2);
        UNIT_ASSERT_VALUES_EQUAL((*groupsJson)[0][1], 3);

        TTextsVector* texts = dynamic_cast<TTextsVector*>(inputContext.at("texts_ctx").Get());
        UNIT_ASSERT_VALUES_EQUAL(texts->Texts.size(), 5);
        UNIT_ASSERT_EQUAL(texts->Texts[0], "abc");
        UNIT_ASSERT_EQUAL(texts->Texts[1], "bca");
        UNIT_ASSERT_EQUAL(texts->Texts[2], "fdt");
        UNIT_ASSERT_EQUAL(texts->Texts[3], "qrtad");
        UNIT_ASSERT_EQUAL(texts->Texts[4], "qrtad2");

        TTextsVector* numbers = dynamic_cast<TTextsVector*>(inputContext.at("numbers_ctx").Get());
        UNIT_ASSERT_VALUES_EQUAL(numbers->Texts.size(), 5);
        UNIT_ASSERT_EQUAL(numbers->Texts[0], "123");
        UNIT_ASSERT_EQUAL(numbers->Texts[1], "321");
        UNIT_ASSERT_EQUAL(numbers->Texts[2], "123");
        UNIT_ASSERT_EQUAL(numbers->Texts[3], "321");
        UNIT_ASSERT_EQUAL(numbers->Texts[4], "535");

        TTextsVector* weights = dynamic_cast<TTextsVector*>(inputContext.at("weight_ctx").Get());
        UNIT_ASSERT_VALUES_EQUAL(weights->Texts.size(), 5);
        UNIT_ASSERT_EQUAL(weights->Texts[0], "0.5");
        UNIT_ASSERT_EQUAL(weights->Texts[1], "0.6");
        UNIT_ASSERT_EQUAL(weights->Texts[2], "0.1");
        UNIT_ASSERT_EQUAL(weights->Texts[3], "0.5");
        UNIT_ASSERT_EQUAL(weights->Texts[4], "0.33");
    }


    Y_UNIT_TEST(TestTGlobalStringToSparseMatrixLayer) {
        THashMap<TUtf16String, size_t> mapping;
        mapping[u"abc"] = 11;
        mapping[u"bcd"] = 22;
        mapping[u"cde"] = 33;

        TTrigramTokenizer* trigramTokenizer = new TTrigramTokenizer(mapping, true);
        TWordTokenizer* wordTokenizer = new TWordTokenizer(mapping, false);

        THashMap<TString, TVector<std::pair<TTokenizerUid, TVector<ui32>>>> remap;

        size_t totalSize = std::max_element(mapping.begin(), mapping.end(),
            [](const auto& lhs, const auto& rhs) {
                return lhs.second < rhs.second;
            })->second + 2;

        auto addRemapInfo = [&remap, totalSize](const TString& name, const ITokenizer* tok, const THashMap<ui32, ui32>& info) {
            TVector<ui32> remapVector(totalSize, tok->DoUseUnknownWord() ? tok->GetUnknownWordId() : Max<ui32>());
            for (const auto& item : info) {
                remapVector[item.first] = item.second;
            }
            remap[name].emplace_back(tok->GetUid(), std::move(remapVector));
        };

        addRemapInfo("out", trigramTokenizer, {{11, 213}});
        addRemapInfo("out1", trigramTokenizer, {{22, 216}, {15, 444}, {33, 1}});
        addRemapInfo("out", wordTokenizer, {{11, 218}, {22, 219}, {18, 999}});

        TGlobalSparsifier* sparsifier = new TGlobalSparsifier({trigramTokenizer, wordTokenizer}, remap);
        TContext context;
        context["sparsifier"].Reset(sparsifier);

        TGlobalStringToSparseMatrixLayer globalStringsToSparseMatrixLayer(
            "titles", "sparsifier", {"out", "out1"});

        auto doTesting = [](auto& layer, const auto& context) {
            layer.Init(context);

            TTextsVector* titles = new TTextsVector();
            titles->Texts = {"abcde", "abc", "bcd"};
            TEvalContext inputContext(&context);
            inputContext["titles"].Reset(titles);

            layer.Apply(inputContext);

            TSparseMatrix* output = (TSparseMatrix*)inputContext.at("out").Get();
            UNIT_ASSERT_VALUES_EQUAL(3, output->Vectors.size());
            UNIT_ASSERT_VALUES_EQUAL(TVector<size_t>({213}), output->Vectors[0].Indexes);
            UNIT_ASSERT_VALUES_EQUAL(TVector<size_t>({213, 218}), output->Vectors[1].Indexes);
            UNIT_ASSERT_VALUES_EQUAL(TVector<size_t>({219}), output->Vectors[2].Indexes);

            TSparseMatrix* output1 = (TSparseMatrix*)inputContext.at("out1").Get();
            UNIT_ASSERT_VALUES_EQUAL(3, output1->Vectors.size());
            UNIT_ASSERT_VALUES_EQUAL(TVector<size_t>({216, 1}), output1->Vectors[0].Indexes);
            UNIT_ASSERT_VALUES_EQUAL(TVector<size_t>(), output1->Vectors[1].Indexes);
            UNIT_ASSERT_VALUES_EQUAL(TVector<size_t>({216}), output1->Vectors[2].Indexes);
        };

        doTesting(globalStringsToSparseMatrixLayer, context);

        // Check after save-load
        TStringStream contextHolder;
        context.Save(&contextHolder);

        TStringStream layerHolder;
        globalStringsToSparseMatrixLayer.Save(&layerHolder);

        TContext loadedContext;
        TBlob contextBlob = TBlob::FromStream(contextHolder);
        loadedContext.Load(contextBlob);

        TGlobalStringToSparseMatrixLayer loadedLayer;
        TBlob layerBlob = TBlob::FromStream(layerHolder);
        loadedLayer.Load(layerBlob);

        doTesting(loadedLayer, loadedContext);
    }

    Y_UNIT_TEST(TestTSparseMatrixToEmbeddingLayer) {
        TSparseMatrix* sparseMatrix = new TSparseMatrix();
        TMatrix* embeddingMatrix = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["sparseMatrix"].Reset(sparseMatrix);
        context["embeddingMatrix"].Reset(embeddingMatrix);

        TSparseMatrixToEmbeddingLayer sparseMatrixToEmbeddingLayer(
            "sparseMatrix", "embeddingMatrix", "outputMatrix");

        sparseMatrixToEmbeddingLayer.Init(context);

        sparseMatrix->Vectors.resize(3);
        sparseMatrix->Vectors[0].Indexes = {0, 3};
        sparseMatrix->Vectors[0].Values = {1.0f, 1.0f};
        sparseMatrix->Vectors[2].Indexes = {2};
        sparseMatrix->Vectors[2].Values = {1.0f};

        embeddingMatrix->Resize(4, 2);
        TVector<float>& data = embeddingMatrix->AsFlatArray();
        for (size_t i = 0; i < 4 * 2; ++i) {
            if (i > 5) {
                data[i] = i - 2;
            } else {
                data[i] = i;
            }
        }

        {
        sparseMatrixToEmbeddingLayer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("outputMatrix").Get();
        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> expectedMatrix = {4.0f, 6.0f, 0.0f, 0.0f, 4.0f, 5.0f};
        UNIT_ASSERT_VALUES_EQUAL(expectedMatrix, output->RepresentAsArray());
        }

        TCharMatrix* charEmbeddingMatrix = new TCharMatrix(4, 2, GetUniformBucketValues(0.0, 5.0, 8));
        TVector<ui8> charData(8);
        for (size_t i = 0; i < 4 * 2; ++i) {
            if (i > 5) {
                charData[i] = (i - 2) * 51;
            } else {
                charData[i] = i * 51;
            }
        }
        charEmbeddingMatrix->SetData(charData);
        context["embeddingMatrix"].Reset(charEmbeddingMatrix);
        sparseMatrixToEmbeddingLayer.Init(context);

        {
            sparseMatrixToEmbeddingLayer.Apply(inputContext);
            TMatrix* output = (TMatrix*)inputContext.at("outputMatrix").Get();
            UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
            UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
            TVector<float> expectedMatrix = {4.0f, 6.0f, 0.0f, 0.0f, 4.0f, 5.0f};
            UNIT_ASSERT_VALUES_EQUAL(expectedMatrix, output->RepresentAsArray());
        }
    }

    Y_UNIT_TEST(TestSparseScaleLayer) {
        TSparseMatrix* input = new TSparseMatrix();
        TMatrix* scaler = new TMatrix(1, 1, 0.81);
        TSparseMatrix* output = new TSparseMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        context["input"].Reset(input);
        context["scaler"].Reset(scaler);
        inputContext["output"].Reset(output);

        TSparseScaleLayer layer("input", "scaler", "output");

        layer.Init(context);

        input->Vectors.resize(3);
        input->Vectors[0].Indexes = {0, 3};
        input->Vectors[0].Values = {0.25f, -3.2f};
        input->Vectors[1].Indexes = {2};
        input->Vectors[1].Values = {3.1f};

        layer.Apply(inputContext);

        UNIT_ASSERT_VALUES_EQUAL(input->Vectors.size(), output->Vectors.size());
        UNIT_ASSERT_VALUES_EQUAL(input->Vectors[0].Values.size(), output->Vectors[0].Values.size());
        UNIT_ASSERT_VALUES_EQUAL(input->Vectors[1].Values.size(), output->Vectors[1].Values.size());

        UNIT_ASSERT_DOUBLES_EQUAL(input->Vectors[0].Values[0] * 0.81, output->Vectors[0].Values[0], 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(input->Vectors[0].Values[1] * 0.81, output->Vectors[0].Values[1], 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(input->Vectors[1].Values[0] * 0.81, output->Vectors[1].Values[0], 1e-5);
    }

    Y_UNIT_TEST(TestSparseRescalerModule) {
        TSparseMatrix* input = new TSparseMatrix();
        TMatrix* w = new TMatrix(1, 4);
        TSparseMatrix* output = new TSparseMatrix();

        (*w)[0][0] = 0.11;
        (*w)[0][1] = -2.1;
        (*w)[0][2] = -0.41;
        (*w)[0][3] = 2.5;

        TContext context;
        TEvalContext inputContext(&context);
        context["input"].Reset(input);
        context["w"].Reset(w);
        inputContext["output"].Reset(output);

        TSparseRescalerLayer layer("input", "w", "output");

        layer.Init(context);

        input->Vectors.resize(3);
        input->Vectors[0].Indexes = {0, 3};
        input->Vectors[0].Values = {0.25f, -3.2f};
        input->Vectors[1].Indexes = {2};
        input->Vectors[1].Values = {3.1f};

        layer.Apply(inputContext);

        auto rescale = [](double val) {
            return 0.11 + -2.1 * val + -0.41 * sqrt(val + 0.01) + 2.5 * log(val + 1);
        };

        UNIT_ASSERT_VALUES_EQUAL(input->Vectors.size(), output->Vectors.size());
        UNIT_ASSERT_VALUES_EQUAL(input->Vectors[0].Values.size(), output->Vectors[0].Values.size());
        UNIT_ASSERT_VALUES_EQUAL(input->Vectors[1].Values.size(), output->Vectors[1].Values.size());

        UNIT_ASSERT_DOUBLES_EQUAL(rescale(input->Vectors[0].Values[0]), output->Vectors[0].Values[0], 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(rescale(input->Vectors[0].Values[1]), output->Vectors[0].Values[1], 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(rescale(input->Vectors[1].Values[0]), output->Vectors[1].Values[0], 1e-5);
    }

    Y_UNIT_TEST(TestTNormalizeRowsLayer) {
        TMatrix* input = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);

        TNormalizeRowsLayer normalizeRowsLayer("input", "output", 0);

        normalizeRowsLayer.Init(context);

        input->Resize(3, 2);
        TVector<float>& data = input->AsFlatArray();
        data = {3.0f, 4.0f, 0.0f, 1.0f, 10.0f, 0.0f};

        normalizeRowsLayer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> expectedData = {3.0f / 5.0f, 4.0f / 5.0f, 0.0, 1.0, 1.0, 0.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTDotLayer) {
        TMatrix* input1 = new TMatrix();
        TMatrix* input2 = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input1"].Reset(input1);
        inputContext["input2"].Reset(input2);

        TDotLayer dotLayer("input1", "input2", "output");

        dotLayer.Init(context);

        input1->Resize(3, 2);
        TVector<float>& data1 = input1->AsFlatArray();
        data1 = {3.0, 4.0, 0.0, 1.0, 10.0, 0.0};
        input2->Resize(3, 2);
        TVector<float>& data2 = input2->AsFlatArray();
        data2 = {-1.0, 1.0, 0.0, 1.0, 5.0, 10.0};

        dotLayer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(1, output->GetNumColumns());
        TVector<float> expectedData = {1.0, 1.0, 50.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTConcatLayer) {
        TMatrix* input1 = new TMatrix();
        TMatrix* input2 = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input1"].Reset(input1);
        inputContext["input2"].Reset(input2);

        TConcatLayer concatLayer({"input1", "input2"}, "output");

        concatLayer.Init(context);

        input1->Resize(3, 2);
        TVector<float>& data1 = input1->AsFlatArray();
        for (size_t i = 0; i < data1.size(); ++i) {
            data1[i] = i;
        }
        input2->Resize(3, 3);
        TVector<float>& data2 = input2->AsFlatArray();
        for (size_t i = 0; i < data2.size(); ++i) {
            data2[i] = i;
        }

        concatLayer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(5, output->GetNumColumns());
        TVector<float> expectedData = {
            0.0, 1.0, 0.0, 1.0, 2.0,
            2.0, 3.0, 3.0, 4.0, 5.0,
            4.0, 5.0, 6.0, 7.0, 8.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTAddLayer) {
        TMatrix* input1 = new TMatrix();
        TMatrix* input2 = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input1"].Reset(input1);
        inputContext["input2"].Reset(input2);

        TAddLayer addLayer({"input1", "input2"}, "output");

        addLayer.Init(context);

        input1->Resize(3, 2);
        TVector<float>& data1 = input1->AsFlatArray();
        for (size_t i = 0; i < data1.size(); ++i) {
            data1[i] = i;
        }
        input2->Resize(3, 2);
        TVector<float>& data2 = input2->AsFlatArray();
        for (size_t i = 0; i < data2.size(); ++i) {
            data2[i] = i;
        }

        addLayer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> expectedData = {
            0.0, 2.0,
            4.0, 6.0,
            8.0, 10.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTWeightedAddLayer) {
        TMatrix* input1 = new TMatrix();
        TMatrix* input2 = new TMatrix();
        TMatrix* weights = new TMatrix();
        weights->Resize(1, 2);
        (*weights)[0][0] = 1;
        (*weights)[0][1] = 2;

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input1"].Reset(input1);
        inputContext["input2"].Reset(input2);
        inputContext["weights"].Reset(weights);

        TWeightedAddLayer addLayer({"input1", "input2"}, "weights", "output");

        addLayer.Init(context);

        input1->Resize(3, 2);
        TVector<float>& data1 = input1->AsFlatArray();
        for (size_t i = 0; i < data1.size(); ++i) {
            data1[i] = i;
        }
        input2->Resize(3, 2);
        TVector<float>& data2 = input2->AsFlatArray();
        for (size_t i = 0; i < data2.size(); ++i) {
            data2[i] = i + 1;
        }

        addLayer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> expectedData = {
            2.0, 5.0,
            8.0, 11.0,
            14.0, 17.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTAddRowLayer) {
        TMatrix* input1 = new TMatrix();
        TMatrix* input2 = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input1);
        context["row"].Reset(input2);

        TAddRowLayer addRowLayer("input", "row", "output");

        addRowLayer.Init(context);

        input1->Resize(3, 2);
        TVector<float>& data1 = input1->AsFlatArray();
        for (size_t i = 0; i < data1.size(); ++i) {
            data1[i] = i;
        }
        input2->Resize(1, 2);
        TVector<float>& data2 = input2->AsFlatArray();
        for (size_t i = 0; i < data2.size(); ++i) {
            data2[i] = i;
        }

        addRowLayer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> expectedData = {
            0.0, 2.0,
            2.0, 4.0,
            4.0, 6.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestMaxPoolingLayer) {
        TMatrix* input = new TMatrix();
        TGenericMatrix<ui64>* groups = new TGenericMatrix<ui64>(1, 4);

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);
        context["groups"].Reset(groups);

        TMaxPoolingLayer layer("input", "groups", "output");

        layer.Init(context);

        input->Resize(4, 2);
        (*input)[0][0] = 2;
        (*input)[1][0] = -3;
        (*input)[2][0] = 4;
        (*input)[3][0] = 2;

        (*input)[0][1] = -5;
        (*input)[1][1] = 2;
        (*input)[2][1] = 0;
        (*input)[3][1] = 3;

        groups->Resize(1, 4);
        (*groups)[0][0] = 1;
        (*groups)[0][1] = 0;
        (*groups)[0][2] = 2;
        (*groups)[0][3] = 1;

        layer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(4, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> expectedData = {
            2.0, -5.0,
            0.0, 0.0,
            4.0, 2.0,
            2.0, 3.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestSumPoolingLayer) {
        TMatrix* input = new TMatrix();
        TGenericMatrix<ui64>* groups = new TGenericMatrix<ui64>(1, 4);
        TMatrix* biases = new TMatrix();
        TMatrix* weights = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);
        inputContext["biases"].Reset(biases);
        inputContext["weights"].Reset(weights);
        context["groups"].Reset(groups);

        input->Resize(4, 2);
        (*input)[0][0] = 2;
        (*input)[1][0] = -3;
        (*input)[2][0] = 4;
        (*input)[3][0] = 2;

        (*input)[0][1] = -5;
        (*input)[1][1] = 2;
        (*input)[2][1] = 0;
        (*input)[3][1] = 3;

        groups->Resize(1, 4);
        (*groups)[0][0] = 1;
        (*groups)[0][1] = 0;
        (*groups)[0][2] = 2;
        (*groups)[0][3] = 1;

        biases->Resize(1, 2);
        (*biases)[0][0] = 2;
        (*biases)[0][1] = -3;

        weights->Resize(4, 1);
        (*weights)[0][0] = 2;
        (*weights)[1][0] = 3;
        (*weights)[2][0] = 1;
        (*weights)[3][0] = -2;

        {
            TSumPoolingLayer layer("input", "groups", "biases", "weights", "output");
            layer.Init(context);

            layer.Apply(inputContext);
            TMatrix* output = (TMatrix*)inputContext.at("output").Get();

            UNIT_ASSERT_VALUES_EQUAL(4, output->GetNumRows());
            UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
            TVector<float> expectedData = {
                6.0, -13.0,
                2.0, -3.0,
                -3.0, 3.0,
                -2.0, -9.0
            };
            UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
        }

        {
            TSumPoolingLayer layer("input", "groups", "biases", "", "output");
            layer.Init(context);

            layer.Apply(inputContext);
            TMatrix* output = (TMatrix*)inputContext.at("output").Get();

            UNIT_ASSERT_VALUES_EQUAL(4, output->GetNumRows());
            UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
            TVector<float> expectedData = {
                4.0, -8.0,
                2.0, -3.0,
                3.0, -1.0,
                4.0, 0.0
            };
            UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
        }
    }

    Y_UNIT_TEST(TestOuterScaleLayer) {
        TMatrix* input = new TMatrix();
        TMatrix* alpha = new TMatrix();
        TMatrix* beta = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);
        context["alpha"].Reset(alpha);
        context["beta"].Reset(beta);

        TOuterScaleLayer layer("input", "alpha", "beta", "output");

        layer.Init(context);

        input->Resize(3, 1);
        (*input)[0][0] = 2;
        (*input)[1][0] = -3;
        (*input)[2][0] = 4;

        alpha->Resize(1, 2);
        (*alpha)[0][0] = 3;
        (*alpha)[0][1] = -2;

        beta->Resize(1, 2);
        (*beta)[0][0] = -4;
        (*beta)[0][1] = 5;

        layer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> expectedData = {
            2.0, 1.0,
            -13.0, 11.0,
            8.0, -3};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTMulRowLayer) {
        TMatrix* input1 = new TMatrix();
        TMatrix* input2 = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input1);
        context["row"].Reset(input2);

        TMulRowLayer mulRowLayer("input", "row", "output");

        mulRowLayer.Init(context);

        input1->Resize(3, 2);
        TVector<float>& data1 = input1->AsFlatArray();
        for (size_t i = 0; i < data1.size(); ++i) {
            data1[i] = i;
        }
        input2->Resize(1, 2);
        TVector<float>& data2 = input2->AsFlatArray();
        for (size_t i = 0; i < data2.size(); ++i) {
            data2[i] = i + 1.0;
        }

        mulRowLayer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> expectedData = {
            0.0, 2.0,
            2.0, 6.0,
            4.0, 10.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTSoftSignLayer) {
        TMatrix* input = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);

        TSoftSignLayer softSignLayer("input", "output", 0.5, 0.5, 1.0);

        softSignLayer.Init(context);

        input->Resize(3, 2);
        TVector<float>& data = input->AsFlatArray();
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = i;
        }

        TSoftSignLayer softSignLayer2;
        TStringStream s;
        softSignLayer.Save(&s);
        TBlob blob = TBlob::FromStream(s);
        softSignLayer2.Load(blob);
        softSignLayer2.Init(context);

        softSignLayer2.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> result = output->RepresentAsArray();
        for (size_t i = 0; i < data.size(); ++i) {
            UNIT_ASSERT(fabs(1.0 + 0.5 * data[i] / (0.5 + fabs(data[i])) - result[i]) < 1e-5);
        }
    }

    Y_UNIT_TEST(TestTRemapLayer) {
        TMatrix* input = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);

        TVector<TVector<float>> remapFrom = {{0.0f, 0.1f, 0.3f, 0.7f, 0.9f, 1.0f}, {0.0f, 0.8f}};
        TVector<TVector<float>> remapTo = {{0.05f, 0.2f, 0.4f, 0.6f, 0.7f, 0.8f}, {0.2f, 0.6f}};

        TRemapLayer remapLayer("input", "output", remapFrom, remapTo);

        remapLayer.Init(context);

        input->Resize(3, 2);
        TVector<float>& data = input->AsFlatArray();
        data = {0.8f, -0.5f, 0.3f, 0.5f, 0.5f, 1.0f};

        TRemapLayer remapLayer2;
        TStringStream s;
        remapLayer.Save(&s);
        TBlob blob = TBlob::FromStream(s);
        remapLayer2.Load(blob);
        remapLayer2.Init(context);

        remapLayer2.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> expectedData = {0.65f, -0.05f, 0.4f, 0.45f, 0.5f, 0.7f};
        TVector<float> result = output->RepresentAsArray();
        UNIT_ASSERT_VALUES_EQUAL(result.size(), expectedData.size());
        for (ui64 i = 0; i < result.size(); ++i) {
            UNIT_ASSERT(fabs(result[i] - expectedData[i]) < 1e-5);
        }
    }

    Y_UNIT_TEST(TestTZeroLayer) {
        TSamplesVector* input = new TSamplesVector();
        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);

        TZeroLayer zeroLayer("input", "output");
        zeroLayer.Init(context);

        TVector<TAtomicSharedPtr<ISample>> samples;
        samples.resize(2);
        samples[0].Reset(new TSample(
            {"query", "title"},
            {"first query", "first title"}));
        samples[1].Reset(new TSample(
            {"query", "title"},
            {"second query", "second title"}));
        input->SetSamples(samples);

        zeroLayer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(1, output->GetNumColumns());
        TVector<float> expectedData = {0, 0};
        TVector<float> result = output->RepresentAsArray();
        UNIT_ASSERT_VALUES_EQUAL(result, expectedData);
    }

    float Cos(const TVector<float>& v1, const TVector<float>& v2) {
        UNIT_ASSERT_VALUES_EQUAL(v1.size(), v2.size());
        float firstNorm2 = 0;
        float secondNorm2 = 0;
        float dot = 0;
        for (size_t i = 0; i < v1.size(); ++i) {
            firstNorm2 += v1[i] * v1[i];
            secondNorm2 += v2[i] * v2[i];
            dot += v1[i] * v2[i];
        }
        return dot / sqrt(firstNorm2) / sqrt(secondNorm2);
    }

    Y_UNIT_TEST(TestTMultiCosLayer) {
        TMatrix* input1 = new TMatrix();
        TMatrix* input2 = new TMatrix();
        TMatrix* weight = new TMatrix();
        TMatrix* bias = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input1"].Reset(input1);
        inputContext["input2"].Reset(input2);
        context["weight"].Reset(weight);
        context["bias"].Reset(bias);

        TMultiCosLayer multCosLayer("input1", "input2", "weight", "bias", "output", 0);

        multCosLayer.Init(context);

        input1->Resize(3, 2);
        TVector<float>& data1 = input1->AsFlatArray();
        data1 = {
            3.0, 4.0,
            0.0, 1.0,
            10.0, 0.0};
        input2->Resize(3, 2);
        TVector<float>& data2 = input2->AsFlatArray();
        data2 = {
            -1.0, 1.0,
            0.0, 1.0,
            5.0, 10.0};

        bias->Resize(1, 2);
        TVector<float>& biasData = bias->AsFlatArray();
        biasData = {
            0.0, 1.0};

        weight->Resize(1, 2);
        TVector<float>& weightData = weight->AsFlatArray();
        weightData = {
            2.0, 1.0};

        multCosLayer.Apply(inputContext);
        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(1, output->GetNumColumns());
        TVector<float> expectedData = {
            Cos({3.0 * 2.0, 4.0 + 1.0}, {-1.0 * 2.0, 1.0 + 1.0}),
            Cos({0.0 * 2.0, 1.0 + 1.0}, {0.0 * 2.0, 1.0 + 1.0}),
            Cos({10.0 * 2.0, 0.0 + 1.0}, {5.0 * 2.0, 10.0 + 1.0})
        };

        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTAffineLayer) {
        TMatrix* input = new TMatrix();
        TMatrix* transform = new TMatrix();
        TMatrix* bias = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);
        context["transform"].Reset(transform);
        context["bias"].Reset(bias);

        TAffineLayer affineLayer("input", "transform", "bias", "output");

        affineLayer.Init(context);

        input->Resize(3, 2);
        TVector<float>& data = input->AsFlatArray();
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = i;
        }

        transform->Resize(2, 2);
        TVector<float>& transformData = transform->AsFlatArray();
        transformData = {0.0, 1.0, 2.0, 3.0};

        bias->Resize(1, 2);
        TVector<float>& biasData = bias->AsFlatArray();
        biasData = {1.0, -1.0};

        affineLayer.Apply(inputContext);

        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());

        TVector<float> expectedData = {2.0, 2.0, 4.0, 12.0, 6.0, 22.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTAddMulLayer) {
        TMatrix* input = new TMatrix();
        TMatrix* addVector = new TMatrix();
        TMatrix* mulVector = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);
        context["addVector"].Reset(addVector);
        context["mulVector"].Reset(mulVector);

        TAddMulLayer addMulLayer("input", "addVector", "mulVector", "output");

        addMulLayer.Init(context);

        input->Resize(3, 2);
        TVector<float>& data = input->AsFlatArray();
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = i;
        }

        addVector->Resize(1, 2);
        TVector<float>& addData = addVector->AsFlatArray();
        addData = {1.0, -2.0};

        mulVector->Resize(1, 2);
        TVector<float>& mulData = mulVector->AsFlatArray();
        mulData = {1.0, 2.0};

        addMulLayer.Apply(inputContext);

        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());

        TVector<float> expectedData = {1.0, -2.0, 3.0, 2.0, 5.0, 6.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTMulAddLayer) {
        TMatrix* input = new TMatrix();
        TMatrix* mulVector = new TMatrix();
        TMatrix* addVector = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);
        context["mulVector"].Reset(mulVector);
        context["addVector"].Reset(addVector);

        TMulAddLayer addMulLayer("input", "mulVector", "addVector", "output");

        addMulLayer.Init(context);

        input->Resize(3, 2);
        TVector<float>& data = input->AsFlatArray();
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = i;
        }

        mulVector->Resize(1, 2);
        TVector<float>& mulData = mulVector->AsFlatArray();
        mulData = {1.0, 2.0};

        addVector->Resize(1, 2);
        TVector<float>& addData = addVector->AsFlatArray();
        addData = {1.0, -2.0};

        addMulLayer.Apply(inputContext);

        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());

        TVector<float> expectedData = {1.0, 0.0, 3.0, 4.0, 5.0, 8.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTSplitLayer) {
        TMatrix* input = new TMatrix(3, 5);
        input->AsFlatArray() = {
            0.0, 1.0, 0.0, 1.0, 2.0,
            2.0, 3.0, 3.0, 4.0, 5.0,
            4.0, 5.0, 6.0, 7.0, 8.0};
        TMatrix* inputs[3] = {new TMatrix(3, 2), new TMatrix(3, 1), new TMatrix(3, 2)};

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);
        inputContext["input0"].Reset(inputs[0]);
        inputContext["input1"].Reset(inputs[1]);
        inputContext["input2"].Reset(inputs[2]);
        TSplitLayer splitLayer("input", {"input0", "input1", "input2"}, {"output0", "output1", "output2"});
        splitLayer.Init(context);
        splitLayer.Apply(inputContext);
        splitLayer.Apply(inputContext);

        TMatrix* outputs[3] = {
              static_cast<TMatrix*>(inputContext.at("output0").Get())
            , static_cast<TMatrix*>(inputContext.at("output1").Get())
            , static_cast<TMatrix*>(inputContext.at("output2").Get())};

        for (size_t i = 0; i < 3; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(inputs[i]->GetNumRows(), outputs[i]->GetNumRows());
            UNIT_ASSERT_VALUES_EQUAL(inputs[i]->GetNumColumns(), outputs[i]->GetNumColumns());
        }

        TVector<float> expectedData0 = {
            0.0, 1.0,
            2.0, 3.0,
            4.0, 5.0};
        TVector<float> expectedData1 = {
            0.0,
            3.0,
            6.0};
        TVector<float> expectedData2 = {
            1.0, 2.0,
            4.0, 5.0,
            7.0, 8.0};
        UNIT_ASSERT_VALUES_EQUAL(outputs[0]->RepresentAsArray(), expectedData0);
        UNIT_ASSERT_VALUES_EQUAL(outputs[1]->RepresentAsArray(), expectedData1);
        UNIT_ASSERT_VALUES_EQUAL(outputs[2]->RepresentAsArray(), expectedData2);
    }

    Y_UNIT_TEST(TestTHadamarLikeProductLayer) {
        TMatrix* inputMatrix = new TMatrix(3, 2);
        inputMatrix->AsFlatArray() = {0, 1, 2, 3, 4, 5};
        TMatrix* inputVector = new TMatrix(3, 1);
        inputVector->AsFlatArray() = {-1, 2, 0.5};

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["inputMatrix"].Reset(inputMatrix);
        inputContext["inputVector"].Reset(inputVector);

        THadamarlikeProductLayer hadamarLayer("inputMatrix", "inputVector", "output");
        hadamarLayer.Init(context);
        hadamarLayer.Apply(inputContext);
        hadamarLayer.Apply(inputContext);

        TMatrix* output = static_cast<TMatrix*>(inputContext.at("output").Get());
        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());
        TVector<float> expectedData = {0, -1, 4, 6, 2, 2.5};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTLinearTransposedLayer) {
        TMatrix* input = new TMatrix();
        TMatrix* transform = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);
        context["transform"].Reset(transform);

        TLinearTransposedLayer linearTransposedLayer("input", "transform", "output");

        linearTransposedLayer.Init(context);

        input->Resize(3, 2);
        TVector<float>& data = input->AsFlatArray();
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = i;
        }

        transform->Resize(2, 2);
        TVector<float>& transformData = transform->AsFlatArray();
        transformData = {0.0, 2.0, 1.0, 3.0};

        linearTransposedLayer.Apply(inputContext);
        linearTransposedLayer.Apply(inputContext);

        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());

        TVector<float> expectedData = {1.0, 3.0, 3.0, 13.0, 5.0, 23.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTJoinMultiplySparsesAndPositionsLayer) {
        TSparseMatrix* sparseInput1 = new TSparseMatrix();
        TSparseMatrix* sparseInput2 = new TSparseMatrix();
        TPositionedOneHotEncodingMatrix* positionInput1 = new TPositionedOneHotEncodingMatrix();
        TPositionedOneHotEncodingMatrix* positionInput2 = new TPositionedOneHotEncodingMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["sparseInput1"].Reset(sparseInput1);
        inputContext["sparseInput2"].Reset(sparseInput2);
        inputContext["positionInput1"].Reset(positionInput1);
        inputContext["positionInput2"].Reset(positionInput2);

        TJoinMultiplySparsesAndPositionsLayer joinLayer(
                {"sparseInput1", "sparseInput2"},
                {"positionInput1", "positionInput2"},
                "outputSparse", "outputPosition"
        );

        joinLayer.Init(context);

        sparseInput1->Vectors.resize(2);
        sparseInput1->Vectors[0].Indexes = {1, 2, 3};
        sparseInput1->Vectors[1].Indexes = {1, 2, 5};
        sparseInput1->Vectors[0].Values = {10, 20, 30};
        sparseInput1->Vectors[1].Values = {10, 20, 50};

        sparseInput2->Vectors.resize(2);
        sparseInput2->Vectors[0].Indexes = {1, 6};
        sparseInput2->Vectors[1].Indexes = {1, 7};
        sparseInput2->Vectors[0].Values = {10, 60};
        sparseInput2->Vectors[1].Values = {10, 70};

        positionInput1->Encodings.resize(2);
        positionInput1->Encodings[0] = {
            TPositionedOneHotEncoding(1, 0, 1, ETokenType::Word),
            TPositionedOneHotEncoding(2, 2, 3, ETokenType::Trigram),
            TPositionedOneHotEncoding(3, 2, 3, ETokenType::Trigram)
        };
        positionInput1->Encodings[1] = {
            TPositionedOneHotEncoding(3, 0, 1, ETokenType::Bigram),
            TPositionedOneHotEncoding(2, 2, 3, ETokenType::Trigram),
            TPositionedOneHotEncoding(4, 5, 6, ETokenType::Trigram)
        };

        positionInput2->Encodings.resize(2);
        positionInput2->Encodings[0] = {
            TPositionedOneHotEncoding(1, 0, 1, ETokenType::Word),
            TPositionedOneHotEncoding(2, 2, 3, ETokenType::Trigram),
            TPositionedOneHotEncoding(6, 2, 3, ETokenType::Trigram)
        };
        positionInput2->Encodings[1] = {};

        joinLayer.Apply(inputContext);
        TSparseMatrix* sparseOutput = VerifyDynamicCast<TSparseMatrix*>(inputContext.at("outputSparse").Get());
        TPositionedOneHotEncodingMatrix* positionsOutput = VerifyDynamicCast<TPositionedOneHotEncodingMatrix*>(inputContext.at("outputPosition").Get());

        UNIT_ASSERT_VALUES_EQUAL(2, sparseOutput->Vectors.size());
        UNIT_ASSERT_VALUES_EQUAL(2, positionsOutput->Encodings.size());
        UNIT_ASSERT_VALUES_EQUAL(sparseOutput->Vectors[0].Indexes, TVector<size_t>({0, 1, 2, 3, 4}));
        UNIT_ASSERT_VALUES_EQUAL(sparseOutput->Vectors[0].Values, TVector<float>({10, 20, 30, 10, 60}));
        UNIT_ASSERT_VALUES_EQUAL(sparseOutput->Vectors[1].Indexes, TVector<size_t>({0, 1, 2, 5, 6}));
        UNIT_ASSERT_VALUES_EQUAL(sparseOutput->Vectors[1].Values, TVector<float>({10, 20, 50, 10, 70}));

        TVector<TVector<TPositionedOneHotEncoding>> expected({{
            TPositionedOneHotEncoding(0, 0, 1, ETokenType::Word),
            TPositionedOneHotEncoding(1, 2, 3, ETokenType::Trigram),
            TPositionedOneHotEncoding(2, 2, 3, ETokenType::Trigram),
            TPositionedOneHotEncoding(3, 0, 1, ETokenType::Word),
            TPositionedOneHotEncoding(5, 2, 3, ETokenType::Trigram),
            TPositionedOneHotEncoding(4, 2, 3, ETokenType::Trigram)
        }, {
            TPositionedOneHotEncoding(3, 0, 1, ETokenType::Bigram),
            TPositionedOneHotEncoding(1, 2, 3, ETokenType::Trigram),
            TPositionedOneHotEncoding(4, 5, 6, ETokenType::Trigram),
        }});
        UNIT_ASSERT_VALUES_EQUAL(expected[0], positionsOutput->Encodings[0]);
        UNIT_ASSERT_VALUES_EQUAL(expected[1], positionsOutput->Encodings[1]);
    }

    Y_UNIT_TEST(TestTRoundToPrecisionLayer) {
        TMatrix* precision = new TMatrix;
        precision->Resize(1, 1);
        (*precision)[0][0] = 4.0f;

        TContext layerContext;
        layerContext["precision"].Reset(precision);

        TRoundToPrecisionLayer layer{"input", "precision", "output"};
        layer.Init(layerContext);

        TMatrix* input = new TMatrix;
        input->Resize(2, 3);
        input->AsFlatArray().assign({
            42.0f, 0.00004f, 0.00005f,
            -42.0f, -0.00004f, -0.00005f,
        });

        TEvalContext inputContext;
        inputContext["input"].Reset(input);
        layer.Apply(inputContext);

        TMatrix* output = VerifyDynamicCast<TMatrix*>(inputContext.at("output").Get());

        UNIT_ASSERT_VALUES_EQUAL(output->GetNumRows(), 2);
        UNIT_ASSERT_VALUES_EQUAL(output->GetNumColumns(), 3);

        const TVector<float> expected{
            42.0f, 0.0f, 0.0001f,
            -42.0f, 0.0f, -0.0001f,
        };
        UNIT_ASSERT_VALUES_EQUAL(output->RepresentAsArray(), expected);

        TMersenne<ui64> rng{42};
        input->Resize(100, 200);
        FillMatrixRandom(input, rng);

        layer.Apply(inputContext);

        UNIT_ASSERT_VALUES_EQUAL(output->GetNumRows(), 100);
        UNIT_ASSERT_VALUES_EQUAL(output->GetNumColumns(), 200);
        for (auto&& i : xrange(100)) {
            for (auto&& j : xrange(200)) {
                UNIT_ASSERT_DOUBLES_EQUAL((*input)[i][j], (*output)[i][j], 1.0e-4);
            }
        }
    }

    Y_UNIT_TEST(TestTBatchNormLayer) {
        TMatrix* input = new TMatrix();
        TMatrix* mean = new TMatrix();
        TMatrix* var = new TMatrix();
        TMatrix* beta = new TMatrix();
        TMatrix* gamma = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);
        context["mean"].Reset(mean);
        context["var"].Reset(var);
        context["beta"].Reset(beta);
        context["gamma"].Reset(gamma);

        TBatchNormLayer bnLayer("input", "mean", "var", "beta", "gamma", "output");

        bnLayer.Init(context);

        input->Resize(3, 2);
        TVector<float>& data = input->AsFlatArray();
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = i;
        }

        mean->Resize(1, 2);
        TVector<float>& meanData = mean->AsFlatArray();
        meanData = {0.0, 1.0};

        var->Resize(1, 2);
        TVector<float>& varData = var->AsFlatArray();
        varData = {0.009, 0.999};

        beta->Resize(1, 2);
        TVector<float>& betaData = beta->AsFlatArray();
        betaData = {0.0, 1.0};

        gamma->Resize(1, 2);
        TVector<float>& gammaData = gamma->AsFlatArray();
        gammaData = {2.0, 1.0};

        bnLayer.Apply(inputContext);

        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());

        TVector<float> expectedData = {0.0, 1.0, 40.0, 3.0, 80.0, 5.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTLinearCombinationLayer) {
        TMatrix* input1 = new TMatrix();
        TMatrix* input2 = new TMatrix();
        TMatrix* input3 = new TMatrix();
        TMatrix* weights = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input1"].Reset(input1);
        inputContext["input2"].Reset(input2);
        inputContext["input3"].Reset(input3);
        context["weights"].Reset(weights);

        TLinearCombinationLayer lincombLayer({"input1", "input2", "input3"},
            "weights", "output");

        lincombLayer.Init(context);

        input1->Resize(2, 2);
        TVector<float>& data1 = input1->AsFlatArray();
        for (size_t i = 0; i < data1.size(); ++i) {
            data1[i] = i;
        }

        input2->Resize(2, 2);
        TVector<float>& data2 = input2->AsFlatArray();
        for (size_t i = 0; i < data2.size(); ++i) {
            data2[i] = i * 2;
        }

        input3->Resize(2, 2);
        TVector<float>& data3 = input3->AsFlatArray();
        for (size_t i = 0; i < data3.size(); ++i) {
            data3[i] = i * 4;
        }

        weights->Resize(3, 2);
        TVector<float>& weightsData = weights->AsFlatArray();
        weightsData = {1.0, 1.0, 0.5, 0.5, 0.25, 0.25};

        lincombLayer.Apply(inputContext);

        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());

        TVector<float> expectedData = {0.0, 3.0, 6.0, 9.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTMatMulLayer) {
        TMatrix* input1 = new TMatrix();
        TMatrix* input2 = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input1"].Reset(input1);
        inputContext["input2"].Reset(input2);

        TMatMulLayer matmulLayer("input1", "input2", "output");

        matmulLayer.Init(context);

        input1->Resize(3, 2);
        TVector<float>& data1 = input1->AsFlatArray();
        for (size_t i = 0; i < data1.size(); ++i) {
            data1[i] = i;
        }

        input2->Resize(2, 2);
        TVector<float>& data2 = input2->AsFlatArray();
        for (size_t i = 0; i < data2.size(); ++i) {
            data2[i] = i;
        }

        matmulLayer.Apply(inputContext);

        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(3, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());

        TVector<float> expectedData = {1.0, 3.0, 3.0, 13.0, 5.0, 23.0};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }

    Y_UNIT_TEST(TestTParamEluLayer) {
        TMatrix* input = new TMatrix();

        TContext context;
        TEvalContext inputContext(&context);
        inputContext["input"].Reset(input);

        TParamEluLayer paramEluLayer("input", "output", 0.5);

        paramEluLayer.Init(context);

        input->Resize(2, 2);
        TVector<float>& data = input->AsFlatArray();
        data = {1.0, 0.0, 10., static_cast<float>(log(0.5))};

        paramEluLayer.Apply(inputContext);

        TMatrix* output = (TMatrix*)inputContext.at("output").Get();

        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(2, output->GetNumColumns());

        TVector<float> expectedData = {1.0, 0.0, 10.0, -0.25};
        UNIT_ASSERT_VALUES_EQUAL(expectedData, output->RepresentAsArray());
    }
}

Y_UNIT_TEST_SUITE(TestSaveLoad) {
    Y_UNIT_TEST(TestTCharMatrix) {
        TStringStream s;

        TCharMatrix matrix(2, 3, GetUniformBucketValues(0.0, 10.0, 8));
        TVector<ui8> data(2 * 3);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = i;
        }

        matrix.SetData(data);

        matrix.Save(&s);

        TCharMatrix matrix2;
        TBlob blob = TBlob::FromStream(s);
        matrix2.Load(blob);

        UNIT_ASSERT_VALUES_EQUAL(2, matrix2.GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(3, matrix2.GetNumColumns());

        ui8 curValue = 0.0;
        for (size_t row = 0; row < 2; ++row) {
            TVector<float> values;
            matrix2.GetRow(row, values);
            for (size_t col = 0; col < 3; ++col) {
                UNIT_ASSERT(fabs(
                    curValue * 10.0f / ((1 << 8) - 1) - values[col]) < 1e-5);
                curValue++;
            }
        }
    }

    Y_UNIT_TEST(TestTMatrix) {
        TMatrix matrix;
        matrix.Resize(2, 3);
        TVector<float>& data = matrix.AsFlatArray();
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = i;
        }

        {
        TStringStream s;
        matrix.Save(&s);

        TBlob blob = TBlob::FromStream(s);
        TMatrix matrix2;
        matrix2.Load(blob);

        UNIT_ASSERT_VALUES_EQUAL(2, matrix2.GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(3, matrix2.GetNumColumns());
        UNIT_ASSERT_VALUES_EQUAL(data, matrix2.RepresentAsArray());
        }
    }

    Y_UNIT_TEST(TestTTrigramTokenizer) {
        THashMap<TUtf16String, size_t> mapping;
        mapping[u"abc"] = 218;
        mapping[u"bcd"] = 57;
        TTrigramTokenizer tringramTokenizer(mapping, true);

        TVector<size_t> tokens;
        TVector<float> values;
        TUtf16String input = u"abcde";
        tringramTokenizer.AddTokens(input, tokens, values);

        TVector<size_t> expectedIds = {218, 57};
        UNIT_ASSERT_VALUES_EQUAL(expectedIds, tokens);

        TStringStream stream;
        tringramTokenizer.Save(&stream);

        TBlob blob = TBlob::FromStream(stream);

        TTrigramTokenizer tokenizer2;
        tokenizer2.Load(blob);

        UNIT_ASSERT_VALUES_EQUAL(true, tokenizer2.NeedLowercaseInput());
        TVector<size_t> tokens2;
        TVector<float> values2;
        tokenizer2.AddTokens(input, tokens2, values2);
        UNIT_ASSERT_VALUES_EQUAL(expectedIds, tokens2);
    }

    Y_UNIT_TEST(TestTBigramsTokenizer) {
        THashMap<TUtf16String, size_t> mapping;
        mapping[u"abc def"] = 218;
        mapping[u"def ghi"] = 57;
        TBigramsTokenizer bigramTokenizer(mapping, true);

        TVector<size_t> tokens;
        TVector<float> values;
        TUtf16String input = u"abc def  ghi jkl";
        bigramTokenizer.AddTokens(input, tokens, values);

        TVector<size_t> expectedIds = {218, 57};
        UNIT_ASSERT_VALUES_EQUAL(expectedIds, tokens);

        TStringStream stream;
        bigramTokenizer.Save(&stream);

        TBlob blob = TBlob::FromStream(stream);

        TBigramsTokenizer tokenizer2;
        tokenizer2.Load(blob);

        UNIT_ASSERT_VALUES_EQUAL(true, tokenizer2.NeedLowercaseInput());
        TVector<size_t> tokens2;
        TVector<float> values2;
        tokenizer2.AddTokens(input, tokens2, values2);
        UNIT_ASSERT_VALUES_EQUAL(expectedIds, tokens2);
    }

    Y_UNIT_TEST(TestTWideBigramsTokenizer) {
        THashMap<TUtf16String, size_t> mapping;
        mapping[u"abc def"] = 218;
        mapping[u"def ghi"] = 57;
        mapping[u"abc ghi"] = 42;
        TWideBigramsTokenizer bigramTokenizer(mapping, true);

        TVector<size_t> tokens;
        TVector<float> values;
        TUtf16String input = u"abc def  ghi jkl";
        bigramTokenizer.AddTokens(input, tokens, values);

        TVector<size_t> expectedIds = {218, 42, 57};
        TVector<float> expectedValues = {1.f, 0.8f, 1.f};

        UNIT_ASSERT_VALUES_EQUAL(expectedIds, tokens);
        UNIT_ASSERT_VALUES_EQUAL(expectedValues, values);

        TStringStream stream;
        bigramTokenizer.Save(&stream);

        TBlob blob = TBlob::FromStream(stream);

        TWideBigramsTokenizer tokenizer2;
        tokenizer2.Load(blob);

        UNIT_ASSERT_VALUES_EQUAL(true, tokenizer2.NeedLowercaseInput());
        TVector<size_t> tokens2;
        TVector<float> values2;
        tokenizer2.AddTokens(input, tokens2, values2);
        UNIT_ASSERT_VALUES_EQUAL(expectedIds, tokens2);
        UNIT_ASSERT_VALUES_EQUAL(expectedValues, values2);
    }

    Y_UNIT_TEST(TestTSparsifier) {
        THashMap<TUtf16String, size_t> mapping;
        mapping[u"abc"] = 218;
        mapping[u"bcd"] = 57;
        TTrigramTokenizer* tringramTokenizer = new TTrigramTokenizer(mapping, true);
        TWordTokenizer* wordTokenizer = new TWordTokenizer(mapping, false);

        TSparsifier sparsifier({tringramTokenizer, wordTokenizer});

        TStringStream stream;
        sparsifier.Save(&stream);

        TBlob blob = TBlob::FromStream(stream);

        TSparsifier sparsifier2;
        sparsifier2.Load(blob);
    }

    Y_UNIT_TEST(TestTGlobalSparsifierSaveLoad) {
        THashMap<TUtf16String, size_t> mapping;
        mapping[u"abc"] = 218;
        mapping[u"bcd"] = 57;
        TTrigramTokenizer* trigramTokenizer = new TTrigramTokenizer(mapping, true);
        TWordTokenizer* wordTokenizer = new TWordTokenizer(mapping, false);

        THashMap<TString, TVector<std::pair<TTokenizerUid, TVector<ui32>>>> remap;
        TVector<ui32> remapVector(
            std::max_element(mapping.begin(), mapping.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.second < rhs.second;
                }
            )->second + 2
        );

        remap["out"].emplace_back(
            trigramTokenizer->GetUid(),
            remapVector
        );

        remap["out"].emplace_back(
            wordTokenizer->GetUid(),
            remapVector
        );

        TGlobalSparsifier sparsifier({trigramTokenizer, wordTokenizer}, remap);

        TStringStream stream;
        sparsifier.Save(&stream);

        TBlob blob = TBlob::FromStream(stream);

        TGlobalSparsifier sparsifier2;
        sparsifier2.Load(blob);
    }

    Y_UNIT_TEST(TestTContext) {
        TMatrix* matrix = new TMatrix();
        matrix->Resize(2, 3);
        TVector<float>& data = matrix->AsFlatArray();
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = i;
        }

        THashMap<TUtf16String, size_t> mapping;
        mapping[u"abc"] = 218;
        mapping[u"bcd"] = 57;

        TTrigramTokenizer* tringramTokenizer = new TTrigramTokenizer(mapping, true);
        TSparsifier* sparsifier = new TSparsifier({tringramTokenizer});

        TContext context;
        context["matrix"].Reset(matrix);
        context["sparsifier"].Reset(sparsifier);
        TStringStream stream;
        context.Save(&stream);

        TContext context2;
        TBlob blob = TBlob::FromStream(stream);
        context2.Load(blob);
    }

    Y_UNIT_TEST(TestGetSubmodel) {
        TMatrixPtr input = new TMatrix();
        TMatrixPtr transform = new TMatrix();
        TMatrixPtr transform2 = new TMatrix();
        TMatrixPtr bias = new TMatrix();

        TModel model;
        model.Parameters["input1"] = input;
        model.Parameters["input2"] = input;
        model.Parameters["transform"] = transform;
        model.Parameters["bias"] = bias;
        model.Parameters["transform2"] = transform2;
        model.Layers.push_back(new TAffineLayer("input1", "transform", "bias", "affine"));
        model.Layers.push_back(new TAffineLayer("input2", "transform2", "bias", "affine2"));
        model.Layers.push_back(new TElementwiseTransform<TRlu>("affine", "rlu"));
        model.Layers.push_back(new TElementwiseTransform<TElu>("rlu1", "elu"));
        model.Inputs.push_back("input1");
        model.Inputs.push_back("input2");
        model.Init();

        auto submodel = model.GetSubmodel("rlu");
        UNIT_ASSERT_VALUES_EQUAL(submodel->Inputs.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(submodel->Inputs[0], "input1");
        UNIT_ASSERT(model.Parameters["input1"].Get() != submodel->Parameters["input1"].Get());
        UNIT_ASSERT(model.Parameters["bias"].Get() == submodel->Parameters["bias"].Get());
        UNIT_ASSERT(model.Parameters["transform"].Get() == submodel->Parameters["transform"].Get());
        UNIT_ASSERT(+submodel->Parameters == 3);
        THashSet<ILayer*> submodelLayers;
        for (auto& l: submodel->Layers) {
            submodelLayers.insert(l.Get());
        }
        for (auto& l: model.Layers) {
            UNIT_ASSERT(!submodelLayers.contains(l.Get()));
        }
        UNIT_ASSERT(submodel->Layers.size() == 2);
        UNIT_ASSERT(model.Blob.Data() == submodel->Blob.Data());
    }

    Y_UNIT_TEST(TestCopyModel) {
        TMatrixPtr input = new TMatrix();
        TMatrixPtr transform = new TMatrix();
        TMatrixPtr transform2 = new TMatrix();
        TMatrixPtr bias = new TMatrix();
        TModel model;
        model.Parameters["input1"] = input;
        model.Parameters["input2"] = input;
        model.Parameters["transform"] = transform;
        model.Parameters["bias"] = bias;
        model.Parameters["transform2"] = transform2;
        model.Layers.push_back(new TAffineLayer("input1", "transform", "bias", "affine"));
        model.Layers.push_back(new TAffineLayer("input2", "transform2", "bias", "affine2"));
        model.Layers.push_back(new TElementwiseTransform<TRlu>("affine", "rlu"));
        model.Layers.push_back(new TElementwiseTransform<TElu>("rlu1", "elu"));
        model.Inputs.push_back("input1");
        model.Inputs.push_back("input2");
        model.Init();
        THashSet<ILayer*> modelLayers;
        for (auto& l: model.Layers) {
            modelLayers.insert(l.Get());
        }

        TModelPtr copyModel = new TModel();
        for (auto i: xrange(2)) {
            if (i == 0) {
                copyModel = new TModel(model);
            } else {
                *copyModel = model;
            }
            UNIT_ASSERT_VALUES_EQUAL(copyModel->Inputs.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(copyModel->Inputs[0], "input1");
            UNIT_ASSERT_VALUES_EQUAL(copyModel->Inputs[1], "input2");
            UNIT_ASSERT(model.Parameters["input1"].Get() != copyModel->Parameters["input1"].Get());
            UNIT_ASSERT(model.Parameters["input2"].Get() != copyModel->Parameters["input2"].Get());
            UNIT_ASSERT(model.Parameters["transform"].Get() == copyModel->Parameters["transform"].Get());
            UNIT_ASSERT(model.Parameters["bias"].Get() == copyModel->Parameters["bias"].Get());
            UNIT_ASSERT(model.Parameters["transform2"].Get() == copyModel->Parameters["transform2"].Get());
            UNIT_ASSERT(+copyModel->Parameters == 5);

            UNIT_ASSERT(copyModel->Layers.size() == 4);
            for (auto& l: copyModel->Layers) {
                UNIT_ASSERT(!modelLayers.contains(l.Get()));
            }
            UNIT_ASSERT(model.Blob.Data() == copyModel->Blob.Data());
        }
    }
}

Y_UNIT_TEST_SUITE(TestGradients) {
    template <class TFunctor>
    void TestGradientOne(TFunctor& f, float* params, const float* analiticalGradients, size_t size,
        float eps=1e-8, float absEps=1e-10, bool expectZero=false)
    {
        bool allSmall = true;
        for (size_t i = 0; i < size; i++) {
            float param = params[i];
            float gradEps = 1.0 / 256;
            params[i] = param + gradEps;
            float plusOutput = f();
            params[i] = param - gradEps;
            float minusOutput = f();
            params[i] = param;
            float numericGradient = (plusOutput - minusOutput) / 2 / gradEps;
            UNIT_ASSERT(!isnan(numericGradient));
            UNIT_ASSERT(!isnan(analiticalGradients[i]));
            if (Abs(analiticalGradients[i] - numericGradient) / (Abs(analiticalGradients[i]) + Abs(numericGradient) + 1e-10) > eps) {
                if (Abs(analiticalGradients[i] - numericGradient) > absEps) {
                        UNIT_ASSERT(false);
                }
            }
            if (Abs(analiticalGradients[i]) > absEps) {
                allSmall = false;
            }
        }
        UNIT_ASSERT_VALUES_EQUAL(expectZero, allSmall);
    }

    //input layer: input -> output
    //grad layer: outputGrad -> inputGrad
    void TestGradient(
            ILayer* inputLayer,
            ILayer* gradLayer,
            TContext& context,
            TEvalContext evalContext,
            TMersenne<ui64>& rng,
            float eps)
    {
        inputLayer->Init(context);
        inputLayer->Apply(evalContext);
        size_t outRows = static_cast<TMatrix*>(evalContext.at("output").Get())->GetNumRows();
        size_t outColumns = static_cast<TMatrix*>(evalContext.at("output").Get())->GetNumColumns();

        TVector<ILayer*> layers;

        TDotLayer dotLayer("output", "dot", "outputDot");
        TElementwiseTransform<TConst1> constLayer("outputDot", "outputDotGrad");
        THadamarlikeProductLayer hadamarLayer("dot", "outputDotGrad", "outputGrad");

        layers.push_back(inputLayer);
        layers.push_back(&dotLayer);
        layers.push_back(&constLayer);
        layers.push_back(&hadamarLayer);
        layers.push_back(gradLayer);

        TMatrix* dot = new TMatrix(outRows, outColumns);
        FillMatrixRandom(dot, rng);
        evalContext["dot"].Reset(dot);

        for (auto layer : layers) {
            layer->Init(context);
            layer->Apply(evalContext);
            layer->Apply(evalContext);
        }

        TMatrix& input = *static_cast<TMatrix*>(evalContext.at("input").Get());
        TMatrix& output = *static_cast<TMatrix*>(evalContext.at("outputDot").Get());
        TMatrix gradient = *static_cast<TMatrix*>(evalContext.at("inputGrad").Get());
        gradient.AcquireData();

        UNIT_ASSERT_VALUES_EQUAL(input.GetNumRows(), gradient.GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(input.GetNumRows(), output.GetNumRows());
        UNIT_ASSERT_VALUES_EQUAL(1, output.GetNumColumns());
        UNIT_ASSERT_VALUES_EQUAL(input.GetNumColumns(), gradient.GetNumColumns());

        for (size_t row = 0; row < input.GetNumRows(); ++row) {
            auto getOutput = [&layers, &evalContext, row]() -> float {
                for (auto layer : layers) {
                    layer->Apply(evalContext);
                    layer->Apply(evalContext);
                }
                for (auto layer : layers) {
                    layer->Apply(evalContext);
                    layer->Apply(evalContext);
                }
                return (*static_cast<TMatrix*>(evalContext.at("outputDot").Get()))[row][0];
            };
            TestGradientOne(getOutput, input[row], gradient[row], input.GetNumColumns(), eps, eps);
        }
    }

    Y_UNIT_TEST(TestTAffineLayerGradient) {
        TMersenne<ui64> rng(42);

        TMatrix* transform = new TMatrix();
        transform->Resize(6, 10);
        FillMatrixRandom(transform, rng);

        TMatrix* bias = new TMatrix();
        bias->Resize(1, 6);
        FillMatrixRandom(bias, rng);

        TContext context;
        context["transform"].Reset(transform);
        context["bias"].Reset(bias);

        TAffineLayer affineLayer("input", "transform", "bias", "output");
        TLinearTransposedLayer gradLayer("outputGrad", "transform", "inputGrad");

        TMatrix* input = new TMatrix();
        input->Resize(5, 10);
        FillMatrixRandom(input, rng);

        TEvalContext evalContext(&context);
        evalContext["input"].Reset(input);

        TestGradient(&affineLayer, &gradLayer, context, evalContext, rng, 1e-3);
    }

    Y_UNIT_TEST(TestTNorm2GradientLayer) {
        TMersenne<ui64> rng(42);

        for (bool doBug : {false, true}) {
            TLayerNorm2 norm2Layer("input", "output", doBug);
            TNorm2GradientLayer gradLayer("input", "outputGrad", "inputGrad", doBug);

            TContext context;

            TMatrix* input = new TMatrix(5, 10);
            FillMatrixRandom(input, rng);

            TEvalContext evalContext(&context);
            evalContext["input"].Reset(input);

            TestGradient(&norm2Layer, &gradLayer, context, evalContext, rng, 1e-3);
        }
    }

    Y_UNIT_TEST(TestTNormalizeRowsGradientLayer) {
        TMersenne<ui64> rng(42);

        TNormalizeRowsLayer normLayer("input", "output", 1e-10);
        TNormalizeRowsGradientLayer gradLayer("input", "output", "outputGrad", "inputGrad", 1e-10);

        TContext context;

        TMatrix* input = new TMatrix(5, 10);
        FillMatrixRandom(input, rng);

        TEvalContext evalContext(&context);
        evalContext["input"].Reset(input);

        TestGradient(&normLayer, &gradLayer, context, evalContext, rng, 1e-3);
    }

    Y_UNIT_TEST(TestTElementwiseGradientLayer) {
        TMersenne<ui64> rng(42);

        TElementwiseTransform<TRlu> rluLayer("input", "output");
        TElementwiseGradientLayer<TRlu> gradLayer("input", "output", "outputGrad", "inputGrad");

        TContext context;

        TMatrix* input = new TMatrix(5, 10);
        FillMatrixRandom(input, rng);

        TEvalContext evalContext(&context);
        evalContext["input"].Reset(input);

        TestGradient(&rluLayer, &gradLayer, context, evalContext, rng, 1e-3);
    }
}
