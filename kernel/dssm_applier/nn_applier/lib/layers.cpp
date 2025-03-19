#include "layers.h"
#include "matrix_prod.h"
#include "saveload_utils.h"

#include <kernel/dssm_applier/nn_applier/lib/optimized_multiply_add/optimized_multiply_add.h>
#include <library/cpp/dot_product/dot_product.h>

#include <util/system/mlock.h>
#include <util/generic/cast.h>
#include <util/generic/xrange.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <util/string/vector.h>
#include <util/string/subst.h>
#include <util/string/escape.h>
#include <util/generic/deque.h>
#include <util/generic/hash_set.h>
#include <util/system/mlock.h>

#include <cmath>

namespace NNeuralNetApplier {

ILayerPtr CreateLayer(const TString& typeName) {
    ILayerPtr curLayer = nullptr;
    if (typeName == "TReLULayer") {
        curLayer = new TElementwiseTransform<TRlu>();
    } else if (typeName == "TELULayer") {
        curLayer = new TElementwiseTransform<TElu>();
    } else if (typeName == "TSELULayer") {
        curLayer = new TElementwiseTransform<TSelu>();
    } else if (typeName.StartsWith("TElementwiseTransform<")) {
        size_t l = TString("TElementwiseTransform<").size();
        TString funcName = typeName.substr(l, typeName.size() - l - 1);
        if (funcName == "rlu") {
            curLayer = new TElementwiseTransform<TRlu>();
        } else if (funcName == "elu") {
            curLayer = new TElementwiseTransform<TElu>();
        } else if (funcName == "selu") {
            curLayer = new TElementwiseTransform<TSelu>();
        } else if (funcName == "tanh") {
            curLayer = new TElementwiseTransform<TTanh>();
        } else if (funcName == "const_1") {
            curLayer = new TElementwiseTransform<TConst1>();
        } else if (funcName == "sigmoid") {
            curLayer = new TElementwiseTransform<TSigmoid>();
        } else {
            ythrow yexception() << "Unknown TElementwiseTransform function: " << funcName;
        }
    } else if (typeName == "TFieldExtractorLayer") {
        curLayer = new TFieldExtractorLayer();
    } else if (typeName == "TSparseScaleLayer") {
        curLayer = new TSparseScaleLayer();
    } else if (typeName == "TStringToSparseMatrixLayer") {
        curLayer = new TStringToSparseMatrixLayer();
    } else if (typeName == "TGlobalStringToSparseMatrixLayer") {
        curLayer = new TGlobalStringToSparseMatrixLayer();
    } else if (typeName == "TSparseMatrixToEmbeddingLayer") {
        curLayer = new TSparseMatrixToEmbeddingLayer();
    } else if (typeName == "TVectorParseLayer") {
        curLayer = new TVectorParseLayer();
    } else if (typeName == "TRepeatedParserLayer") {
        curLayer = new TRepeatedParserLayer<TJsonParser>();
    } else if (typeName == "TRepeatedBinaryParserLayer") {
        curLayer = new TRepeatedParserLayer<TBinaryParser>();
    } else if (typeName == "TMaxPoolingLayer") {
        curLayer = new TMaxPoolingLayer();
    } else if (typeName == "TSumPoolingLayer") {
        curLayer = new TSumPoolingLayer();
    } else if (typeName == "TColumnElemwiseTransformLayer") {
        curLayer = new TColumnElemwiseTransformLayer();
    } else if (typeName == "TNormalizeRowsLayer") {
        curLayer = new TNormalizeRowsLayer();
    } else if (typeName == "TLayerNorm") {
        curLayer = new TLayerNorm();
    } else if (typeName == "TLayerNorm2") {
        curLayer = new TLayerNorm2();
        static_cast<TLayerNorm2*>(curLayer.Get())->SetDefaultDoBug(true);
    } else if (typeName == "TLayerNorm2Fixed") {
        curLayer = new TLayerNorm2();
    } else if (typeName == "TDotLayer") {
        curLayer = new TDotLayer();
    } else if (typeName == "TConcatLayer") {
        curLayer = new TConcatLayer();
    } else if (typeName == "TAddLayer") {
        curLayer = new TAddLayer();
    } else if (typeName == "TWeightedAddLayer") {
        curLayer = new TWeightedAddLayer();
    } else if (typeName == "TMultiCosLayer") {
        curLayer = new TMultiCosLayer();
    } else if (typeName == "TAffineLayer") {
        curLayer = new TAffineLayer();
    } else if (typeName == "TScaleLayer") {
        curLayer = new TScaleLayer();
    } else if (typeName == "TPreDotScaleLayer") {
        curLayer = new TPreDotScaleLayer();
    } else if (typeName == "TShallowCopyLayer") {
        curLayer = new TShallowCopyLayer();
    } else if (typeName == "TAddRowLayer") {
        curLayer = new TAddRowLayer();
    } else if (typeName == "TOuterScaleLayer") {
        curLayer = new TOuterScaleLayer();
    } else if (typeName == "TMulRowLayer") {
        curLayer = new TMulRowLayer();
    } else if (typeName == "TAddMulLayer") {
        curLayer = new TAddMulLayer();
    } else if (typeName == "TMulAddLayer") {
        curLayer = new TMulAddLayer();
    } else if (typeName == "TSoftSignLayer") {
        curLayer = new TSoftSignLayer();
    } else if (typeName == "TQuantizeLayer") {
        curLayer = new TQuantizeLayer();
    } else if (typeName == "TSoftmaxQuantizeLayer") {
        curLayer = new TSoftmaxQuantizeLayer();
    } else if (typeName == "TCompressorLayer") {
        curLayer = new TCompressorLayer();
    } else if (typeName == "TDecompressorLayer") {
        curLayer = new TDecompressorLayer();
    } else if (typeName == "TOneHotCompressorLayer") {
        curLayer = new TOneHotCompressorLayer();
    } else if (typeName == "TOneHotDecompressorLayer") {
        curLayer = new TOneHotDecompressorLayer();
    } else if (typeName == "TGroupSoftmaxLayer") {
        curLayer = new TGroupSoftmaxLayer();
    } else if (typeName == "TStringToSparseMatrixAndPosLayer") {
        curLayer = new TStringToSparseMatrixAndPosLayer();
    } else if (typeName == "THadamarlikeProductLayer") {
        curLayer = new THadamarlikeProductLayer();
    } else if (typeName == "TNormalizeRowsGradientLayer") {
        curLayer = new TNormalizeRowsGradientLayer();
    } else if (typeName == "TNorm2GradientLayerFixed") {
        curLayer = new TNorm2GradientLayer();
    } else if (typeName == "TSparseRescalerLayer") {
        curLayer = new TSparseRescalerLayer();
    } else if (typeName == "TLinearTransposedLayer") {
        curLayer = new TLinearTransposedLayer();
    } else if (typeName.StartsWith("TElementwiseGradientLayer<")) {
        size_t l = TString("TElementwiseGradientLayer<").size();
        TString funcName = typeName.substr(l, typeName.size() - l - 1);
        if (funcName == "rlu") {
            curLayer = new TElementwiseGradientLayer<TRlu>();
        } else {
            ythrow yexception() << "Unknown TElementwiseGradientLayer function: " << funcName;
        }
    } else if (typeName == "TSparseMatrixToEmbeddingGradientLayer") {
        curLayer = new TSparseMatrixToEmbeddingGradientLayer();
    } else if (typeName == "TJoinSparseGradsLayer") {
        curLayer = new TJoinSparseGradsLayer();
    } else if (typeName.StartsWith("TSparseMatrixStatisticLayer<")) {
        size_t l = TString("TSparseMatrixStatisticLayer<").size();
        TString funcName = typeName.substr(l, typeName.size() - l - 1);
        if (funcName == "max") {
            curLayer = new TSparseMatrixStatisticLayer<NAggregateFunctions::TMax>();
        } else if (funcName == "min") {
            curLayer = new TSparseMatrixStatisticLayer<NAggregateFunctions::TMin>();
        } else if (funcName == "std") {
            curLayer = new TSparseMatrixStatisticLayer<NAggregateFunctions::TStd>();
        } else if (funcName == "mean") {
            curLayer = new TSparseMatrixStatisticLayer<NAggregateFunctions::TMean>();
        } else if (funcName == "moment3_central") {
            curLayer = new TSparseMatrixStatisticLayer<NAggregateFunctions::TMoment3Central>();
        } else {
            ythrow yexception() << "Unknown TSparseMatrixStatisticLayer function: " << funcName;
        }
    } else if (typeName == "TSplitLayer") {
        curLayer = new TSplitLayer();
    } else if (typeName == "TRemapLayer") {
        curLayer = new TRemapLayer();
    } else if (typeName == "TZeroLayer") {
        curLayer = new TZeroLayer();
    } else if (typeName == "TJoinMultiplySparsesAndPositionsLayer") {
        curLayer = new TJoinMultiplySparsesAndPositionsLayer();
    } else if (typeName == "TRoundToPrecisionLayer") {
        curLayer = new TRoundToPrecisionLayer{};
    } else if (typeName == "TBatchNormLayer") {
        curLayer = new TBatchNormLayer();
    } else if (typeName == "TLinearCombinationLayer") {
        curLayer = new TLinearCombinationLayer();
    } else if (typeName == "TMatMulLayer") {
        curLayer = new TMatMulLayer();
    } else if (typeName == "TParamEluLayer") {
        curLayer = new TParamEluLayer();
    } else if (typeName.StartsWith("TElementwiseBinaryTransform<")) {
        size_t l = TString("TElementwiseBinaryTransform<").size();
        TString funcName = typeName.substr(l, typeName.size() - l - 1);
        if (funcName == "mul") {
            curLayer = new TElementwiseBinaryTransform<TMul>();
        } else if (funcName == "absdiff") {
            curLayer = new TElementwiseBinaryTransform<TAbsDiff>();
        } else {
            ythrow yexception() << "Unknown TElementwiseBinaryTransform function: " << funcName;
        }
    } else {
        ythrow yexception() << "Unknown ILayer implementation type: " << typeName;
    } 
    return curLayer;
}

ILayerPtr ILayer::Clone() const {
    TStringStream ss;
    Save(&ss);
    TBlob b = TBlob::FromString(ss.Str());
    ILayerPtr res = CreateLayer(GetTypeName());
    res->Load(b);
    return res;
}

void CheckSameRows(const TMatrix* matrix1, const TMatrix* matrix2,
    const TString& matrixName1, const TString& matrixName2)
{
    if (matrix1->GetNumRows() != matrix2->GetNumRows()) {
        ythrow yexception() << "Different number of rows in matrices "
            << matrixName1 << " and " << matrixName2 << ": "
            << matrix1->GetNumRows() << " != "
            << matrix2->GetNumRows();
    }
}

void CheckSameColumns(const TMatrix* matrix1, const TMatrix* matrix2,
    const TString& matrixName1, const TString& matrixName2)
{
    if (matrix1->GetNumColumns() != matrix2->GetNumColumns()) {
        ythrow yexception() << "Different number of columns in matrices "
            << matrixName1 << " and " << matrixName2 << ": "
            << matrix1->GetNumColumns() << " != "
            << matrix2->GetNumColumns();
    }
}

void CheckSameSize(const TMatrix* matrix1, const TMatrix* matrix2,
    const TString& matrixName1, const TString& matrixName2)
{
    CheckSameRows(matrix1, matrix2, matrixName1, matrixName2);
    CheckSameColumns(matrix1, matrix2, matrixName1, matrixName2);
}

void TryRename(const TString& name, const TString& newName, TString& variableName) {
    if (variableName == name) {
        variableName = newName;
    }
}

template<class T, class ContextType>
T CheckInputAndCast(const ContextType& context, const TString& input) {
    return VerifyDynamicCast<T>(context.at(input).Get());
}

template <class TFunc>
void TElementwiseTransform<TFunc>::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    ::Save(s, fields);
}

template <class TFunc>
void TElementwiseTransform<TFunc>::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
}

template <class TFunc>
void TElementwiseTransform<TFunc>::Init(const TContext& context) {
    Y_UNUSED(context);
}

template <class TFunc>
void TElementwiseTransform<TFunc>::Apply(TEvalContext& context) const {
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());
    const float* inputData = (*inputMatrix)[0];
    float* outputData = (*outputMatrix)[0];
    for (size_t i = 0; i < inputMatrix->GetFlatSize(); ++i) {
        outputData[i] = TFunc::Fprop(inputData[i]);
    }
}

template <class TFunc>
void TElementwiseTransform<TFunc>::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

template class TElementwiseTransform<TRlu>;
template class TElementwiseTransform<TTanh>;
template class TElementwiseTransform<TElu>;
template class TElementwiseTransform<TSelu>;
template class TElementwiseTransform<TSigmoid>;
template class TElementwiseTransform<TLinear>;
template class TElementwiseTransform<TSoftplus>;

void TColumnElemwiseTransformLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Funcs"] = SaveToStroka(Funcs);
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TColumnElemwiseTransformLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    LoadFromStroka(fields.at("Funcs"), &Funcs);
    Output = fields.at("Output");
}

void TColumnElemwiseTransformLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

namespace {
    template <class TFunc>
    void ApplyFunc2Column(const float* input, float* output, ui64 numRows, ui64 numColumns) {
        for (auto i : xrange(numRows)) {
            Y_UNUSED(i);
            *output = TFunc::Fprop(*input);
            input += numColumns;
            output += numColumns;
        }
    }

    void ApplyFunc2Column(EFunction f, const float* input, float* output, ui64 numRows, ui64 numColumns) {
        switch (f) {
            case EFunction::Elu:
                ApplyFunc2Column<TElu>(input, output, numRows, numColumns);
                break;
            case EFunction::Rlu:
                ApplyFunc2Column<TRlu>(input, output, numRows, numColumns);
                break;
            case EFunction::Tanh:
                ApplyFunc2Column<TTanh>(input, output, numRows, numColumns);
                break;
            case EFunction::Sigmoid:
                ApplyFunc2Column<TSigmoid>(input, output, numRows, numColumns);
                break;
            case EFunction::Linear:
                ApplyFunc2Column<TLinear>(input, output, numRows, numColumns);
                break;
            case EFunction::Softplus:
                ApplyFunc2Column<TSoftplus>(input, output, numRows, numColumns);
                break;
            case EFunction::Selu:
                ApplyFunc2Column<TSelu>(input, output, numRows, numColumns);
                break;
            default:
                Cerr << "Unknown function " << (int)f << Endl;
                Y_VERIFY(false);
        }
    }
}

void TColumnElemwiseTransformLayer::Apply(TEvalContext& context) const {
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);
    Y_ENSURE(Funcs.size() == inputMatrix->GetNumColumns());

    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());
    const float* inputData = (*inputMatrix)[0];
    float* outputData = (*outputMatrix)[0];
    for (size_t i = 0; i < inputMatrix->GetNumColumns(); ++i) {
        ApplyFunc2Column(Funcs[i], inputData + i, outputData + i, inputMatrix->GetNumRows(),
            inputMatrix->GetNumColumns());
    }
}

void TColumnElemwiseTransformLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

TFieldExtractorLayer::TFieldExtractorLayer(const TString& input,
    const THashMap<TString, TString>& annotationToOutput)
    : Input(input)
    , AnnotationToOutput(annotationToOutput)
{
}

TFieldExtractorLayer::TFieldExtractorLayer(const TString& input, const TString& inputField,
    const TString& outputField
)
    : Input(input)
{
    AnnotationToOutput[inputField] = outputField;
}

TString TFieldExtractorLayer::GetName() const {
    TStringStream ss("TFieldExtractorLayer<");
    bool first = true;
    for (const auto& item : AnnotationToOutput) {
        if (!first) {
            ss << ", ";
        }
        ss << item.first;
        first = false;
    }
    ss << ">";
    return ss.Str();
}

void TFieldExtractorLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["AnnotationToOutput"] = SaveToStroka(AnnotationToOutput);
    ::Save(s, fields);
}

void TFieldExtractorLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    LoadFromStroka<THashMap<TString, TString>>(
        fields.at("AnnotationToOutput"), &AnnotationToOutput);
}

TVector<TString> TFieldExtractorLayer::GetInputs() const {
    return {Input};
}

TVector<TString> TFieldExtractorLayer::GetOutputs() const {
    TVector<TString> result;
    for (auto& it : AnnotationToOutput) {
        result.push_back(it.second);
    }
    return result;
}

void TFieldExtractorLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TFieldExtractorLayer::Apply(TEvalContext& context) const {
    THashMap<TString, TTextsVector*> annotationToTextVector;
    for (auto& it : AnnotationToOutput) {
        const TString& annotation = it.first;
        const TString& output = it.second;
        annotationToTextVector[annotation] = context.CreateOrGet<TTextsVector>(output);
    }

    TSamplesVector* inputSamplesVector = CheckInputAndCast<TSamplesVector*>(context, Input);
    const TVector<TAtomicSharedPtr<ISample>>& samples = inputSamplesVector->GetSamples();
    for (auto& it : annotationToTextVector) {
        it.second->Texts.resize(samples.size());
    }

    for (size_t sampleId = 0; sampleId < samples.size(); ++sampleId) {
        ExtractFieldsFromSample(samples[sampleId], sampleId, annotationToTextVector);
    }
}

void TFieldExtractorLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    THashMap<TString, TString> newAnnotationToOutput;
    for (const auto& it : AnnotationToOutput) {
        newAnnotationToOutput[it.first] = (it.second == name ? newName : it.second);
    }
    AnnotationToOutput = newAnnotationToOutput;
}

bool TFieldExtractorLayer::RenameField(const TString& name, const TString& newName) {
    THashMap<TString, TString> newAnnotationToOutput;
    bool success = false;
    for (const auto& it : AnnotationToOutput) {
        newAnnotationToOutput[it.first == name ? newName : it.first] = it.second;
        success |= it.first == name;
    }
    AnnotationToOutput = newAnnotationToOutput;
    return success;
}

void TFieldExtractorLayer::ExtractFieldsFromSample(const TAtomicSharedPtr<ISample>& sample, size_t sampleId,
    THashMap<TString, TTextsVector*>& annotationToTextVector) const
{
    const TVector<TString>& annotations = sample->GetAnnotations();
    const TVector<TString>& features = sample->GetFeatures();

    ui64 coveredFields = 0;

    for (size_t i = 0; i < annotations.size(); ++i) {
        const TString& annotation = annotations[i];
        const TString& str = features[i];
        auto it = annotationToTextVector.find(annotation);
        if (it == annotationToTextVector.end()) {
            continue;
        }
        TTextsVector* outputVector = it->second;
        outputVector->Texts[sampleId] = str;
        coveredFields++;
    }
    if (coveredFields != annotationToTextVector.size()) {
        TVector<TString> requiredAnnotations;
        for (const auto& it : annotationToTextVector) {
            requiredAnnotations.push_back(it.first);
        }

        ythrow yexception() << "Not all fields required by the model are covered by the sample: "
            << "Required annotations = " << JoinStrings(requiredAnnotations, ",") << "; "
            << "Actual annotations = " << JoinStrings(annotations, ",");
    }
}

TStringToSparseMatrixLayer::TStringToSparseMatrixLayer(const TString& textsInput,
    const TString& sparsifierInput,
    const TString& output)
    : TextsInput(textsInput)
    , SparsifierInput(sparsifierInput)
    , Output(output)
{
}

void TStringToSparseMatrixLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["TextsInput"] = TextsInput;
    fields["SparsifierInput"] = SparsifierInput;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TStringToSparseMatrixLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    TextsInput = fields.at("TextsInput");
    SparsifierInput = fields.at("SparsifierInput");
    Output = fields.at("Output");
}

void TStringToSparseMatrixLayer::Init(const TContext& context) {
    Sparsifier = CheckInputAndCast<TSparsifier*>(context, SparsifierInput);
}

void TStringToSparseMatrixLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TSparseMatrix();
    }
    TTextsVector* textsVector = CheckInputAndCast<TTextsVector*>(context, TextsInput);
    TSparseMatrix* outputSparseMatrix = CheckInputAndCast<TSparseMatrix*>(context, Output);

    const TVector<TString>& texts = textsVector->Texts;

    outputSparseMatrix->Vectors.resize(texts.size());

    for (size_t textId = 0; textId < texts.size(); ++textId) {
        outputSparseMatrix->Vectors[textId].Indexes.clear();
        outputSparseMatrix->Vectors[textId].Values.clear();

        const TString& curText = texts[textId];
        Sparsifier->ToSparse(curText, outputSparseMatrix->Vectors[textId]);
    }
}

void TStringToSparseMatrixLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, TextsInput);
    TryRename(name, newName, SparsifierInput);
    TryRename(name, newName, Output);
}

// ---------------------------global to sparse layer-----------------------------
TGlobalStringToSparseMatrixLayer::TGlobalStringToSparseMatrixLayer(const TString& textsInput,
    const TString& sparsifierInput,
    const TVector<TString>& output)
    : TextsInput(textsInput)
    , SparsifierInput(sparsifierInput)
    , Output(output)
{
}

void TGlobalStringToSparseMatrixLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["TextsInput"] = TextsInput;
    fields["SparsifierInput"] = SparsifierInput;
    fields["Output"] = SaveToStroka(Output);
    ::Save(s, fields);
}

TAtomicSharedPtr<ILayer> TGlobalStringToSparseMatrixLayer::CloneForOutput(const THashSet<TString>& usefulVariables) const {
    THolder<TGlobalStringToSparseMatrixLayer> res = MakeHolder<TGlobalStringToSparseMatrixLayer>();
    res->TextsInput = TextsInput;
    res->SparsifierInput = SparsifierInput;
    for (const TString& o : Output)
        if (usefulVariables.contains(o))
            res->Output.push_back(o);
    return res.Release();
}

void TGlobalStringToSparseMatrixLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    TextsInput = fields.at("TextsInput");
    SparsifierInput = fields.at("SparsifierInput");
    LoadFromStroka(fields.at("Output"), &Output);
}

void TGlobalStringToSparseMatrixLayer::Init(const TContext& context) {
    Sparsifier = CheckInputAndCast<TGlobalSparsifier*>(context, SparsifierInput);
    Sparsifier->MapOutputs(Output, OutputIndices);
}

void TGlobalStringToSparseMatrixLayer::Apply(TEvalContext& context) const {
    TTextsVector* textsVector = CheckInputAndCast<TTextsVector*>(context, TextsInput);
    const TVector<TString>& texts = textsVector->Texts;

    TVector<std::pair<size_t, TSparseVector&>> resultWrapper;
    resultWrapper.reserve(Output.size());

    TVector<TSparseMatrix*> outMatrixes(Reserve(Output.size()));
    for (const auto& out : Output) {
        outMatrixes.push_back(context.CreateOrGet<TSparseMatrix>(out));
        outMatrixes.back()->Vectors.resize(texts.size());
    }

    TVector<TGlobalSparsifier::TPairOfIndexesAndValues> tokenizerResultsBuffer;

    for (size_t textId = 0; textId < texts.size(); ++textId) {
        resultWrapper.clear();
        for (size_t i = 0; i < Output.size(); ++i) {
            TSparseMatrix* outputSparseMatrix = outMatrixes[i];
            outputSparseMatrix->Vectors[textId].Indexes.clear();
            outputSparseMatrix->Vectors[textId].Values.clear();
            resultWrapper.emplace_back(OutputIndices[i], outputSparseMatrix->Vectors[textId]);
        }

        const TString& curText = texts[textId];
        Sparsifier->ToSparse(curText, resultWrapper, tokenizerResultsBuffer);
    }
}

void TGlobalStringToSparseMatrixLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, TextsInput);
    TryRename(name, newName, SparsifierInput);
    for (auto& out : Output) {
        TryRename(name, newName, out);
    }
    Sparsifier->RenameVariable(name, newName);
}

// -----------------------------sparse scaler--------------------
TSparseScaleLayer::TSparseScaleLayer(const TString& input, const TString& scalerInput,
    const TString& output)
    : Input(input)
    , Scaler(scalerInput)
    , Output(output)
{}

void TSparseScaleLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Scaler"] = Scaler;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TSparseScaleLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Scaler = fields.at("Scaler");
    Output = fields.at("Output");
}

void TSparseScaleLayer::Init(const TContext& context) {
    ScalerPtr = CheckInputAndCast<TMatrix*>(context, Scaler);
    Y_ASSERT(ScalerPtr->GetSize() == 1);
}

void TSparseScaleLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TSparseMatrix();
    }
    TSparseMatrix* input = CheckInputAndCast<TSparseMatrix*>(context, Input);
    TSparseMatrix* output = CheckInputAndCast<TSparseMatrix*>(context, Output);
    float scaler = (*ScalerPtr)[0][0];
    output->Vectors.resize(input->Vectors.size());

    for (const auto i: xrange(input->Vectors.size())) {
        output->Vectors[i] = input->Vectors[i];
        for (auto& v : output->Vectors[i].Values) {
            v *= scaler;
        }
    }
}

void TSparseScaleLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Scaler);
    TryRename(name, newName, Output);
}

// ---------------------------------------------------------------

TSparseMatrixToEmbeddingLayer::TSparseMatrixToEmbeddingLayer(
    const TString& sparseMatrixInput,
    const TString& embeddingMatrixInput,
    const TString& output)
    : SparseMatrixInput(sparseMatrixInput)
    , EmbeddingMatrixInput(embeddingMatrixInput)
    , Output(output)
{
}

void TSparseMatrixToEmbeddingLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["SparseMatrixInput"] = SparseMatrixInput;
    fields["EmbeddingMatrixInput"] = EmbeddingMatrixInput;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TSparseMatrixToEmbeddingLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    SparseMatrixInput = fields.at("SparseMatrixInput");
    EmbeddingMatrixInput = fields.at("EmbeddingMatrixInput");
    Output = fields.at("Output");
}

void TSparseMatrixToEmbeddingLayer::Init(const TContext& context) {
    TString embeddingTypeName = context.at(EmbeddingMatrixInput)->GetTypeName();
    if (embeddingTypeName == "TMatrix") {
        EmbeddingMatrix = CheckInputAndCast<TMatrix*>(context, EmbeddingMatrixInput);
        CharEmbeddingMatrix = nullptr;
    } else if (embeddingTypeName == "TCharMatrix") {
        EmbeddingMatrix = nullptr;
        CharEmbeddingMatrix = CheckInputAndCast<TCharMatrix*>(context, EmbeddingMatrixInput);
    } else {
        ythrow yexception() << "Bad embedding matrix type: " << embeddingTypeName;
    }
}

void TSparseMatrixToEmbeddingLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);

    TSparseMatrix* sparseMatrix = CheckInputAndCast<TSparseMatrix*>(context, SparseMatrixInput);
    const TVector<TSparseVector>& vectors = sparseMatrix->Vectors;

    size_t embeddingColumns = EmbeddingMatrix != nullptr
        ? EmbeddingMatrix->GetNumColumns()
        : CharEmbeddingMatrix->GetNumColumns();
    size_t embeddingRows = EmbeddingMatrix != nullptr
        ? EmbeddingMatrix->GetNumRows()
        : CharEmbeddingMatrix->GetNumRows();

    outputMatrix->Resize(vectors.size(), embeddingColumns);
    outputMatrix->FillZero();

    if (CharEmbeddingMatrix && CharEmbeddingMatrix->IsUniformPacked()) {
        auto multImpl = GetImplForDoMultiplyAddWithUnpackBatched(embeddingColumns);
        for (size_t inputId = 0; inputId < vectors.size(); ++inputId) {
            Y_ASSERT(vectors[inputId].Indexes.size() == vectors[inputId].Values.size());
            float* outputRow = (*outputMatrix)[inputId];
            multImpl(
                embeddingColumns,
                {CharEmbeddingMatrix->GetData(), CharEmbeddingMatrix->GetSize()},
                {outputRow, outputRow + CharEmbeddingMatrix->GetNumColumns()},
                vectors[inputId].Values,
                vectors[inputId].Indexes,
                CharEmbeddingMatrix->GetReconstructedCoeff(),
                CharEmbeddingMatrix->GetReconstructedMin()
            );
        }
        return;
    }
    for (size_t inputId = 0; inputId < vectors.size(); ++inputId) {
        Y_ASSERT(vectors[inputId].Indexes.size() == vectors[inputId].Values.size());
        float* outputRow = (*outputMatrix)[inputId];
        for (size_t i = 0; i < vectors[inputId].Indexes.size(); ++i) {
            size_t index =  vectors[inputId].Indexes[i];
            if (index >= embeddingRows) {
                continue;
            }
            float value = vectors[inputId].Values[i];
            float* embeddingRow = nullptr;
            if (EmbeddingMatrix != nullptr) {
               embeddingRow = (*EmbeddingMatrix)[index];
            } else {
                CharEmbeddingMatrix->MutliplyAndAddRowTo(index, value, outputRow);
                continue;
            }
            // TODO(agusakov): Speed this up if needed.
            for (size_t column = 0; column < embeddingColumns; ++column) {
                outputRow[column] += embeddingRow[column] * value;
            }
        }
    }
}

void TSparseMatrixToEmbeddingLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, SparseMatrixInput);
    TryRename(name, newName, EmbeddingMatrixInput);
    TryRename(name, newName, Output);
}

TVectorParseLayer::TVectorParseLayer(const TString& input,
    const TString& output,
    size_t size,
    char sep)
    : Input(input)
    , Output(output)
    , Size(size)
    , Separator(sep)
{
}

void TVectorParseLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["Size"] = SaveToStroka(Size);
    fields["Separator"] = SaveToStroka(Separator);
    ::Save(s, fields);
}

void TVectorParseLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka(fields.at("Size"), &Size);
    LoadFromStroka(fields.at("Separator"), &Separator);
}

void TVectorParseLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TVectorParseLayer::Apply(TEvalContext& context) const {
    TTextsVector* textsVector = CheckInputAndCast<TTextsVector*>(context, Input);
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);

    const TVector<TString>& texts = textsVector->Texts;

    outputMatrix->Resize(texts.size(), Size);

    for (size_t textId = 0; textId < texts.size(); ++textId) {
        TStringBuf txtVector = texts[textId];
        size_t idx = 0;
        TStringBuf token;
        while (txtVector.NextTok(Separator, token)) {
            Y_ENSURE(idx < Size, "Input float vector size is greater than expected");
            (*outputMatrix)[textId][idx] = FromString<float>(token);
            ++idx;
        }
        Y_ENSURE(idx == Size, "Got input vector of size " << idx << ", expected size " << Size << " for layer: " << Output);
    }
}

void TVectorParseLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

template<class FormatParser>
void TRepeatedParserLayer<FormatParser>::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["OutputFields"] = SaveToStroka(OutputFields);
    fields["OutputNames"] = SaveToStroka(OutputNames);
    fields["GroupsSizesName"] = GroupsSizesName;
    ::Save(s, fields);
}

template<class FormatParser>
void TRepeatedParserLayer<FormatParser>::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    LoadFromStroka(fields.at("OutputFields"), &OutputFields);
    LoadFromStroka(fields.at("OutputNames"), &OutputNames);
    GroupsSizesName = fields.at("GroupsSizesName");
}

template<class FormatParser>
void TRepeatedParserLayer<FormatParser>::Init(const TContext& context) {
    Y_UNUSED(context);
}

template<class FormatParser>
void TRepeatedParserLayer<FormatParser>::Apply(TEvalContext& context) const {
    TTextsVector* inputVector = CheckInputAndCast<TTextsVector*>(context, Input);
    TVector<TTextsVector*> outputVectors(Reserve(OutputNames.size()));
    for (const auto& item : OutputNames) {
        outputVectors.push_back(context.CreateOrGet<TTextsVector>(item));
        outputVectors.back()->Texts.clear();
    }
    TGenericMatrix<ui64>* outputGroups = context.CreateOrGet<TGenericMatrix<ui64>>(GroupsSizesName);
    outputGroups->Resize(1, inputVector->Texts.size());

    FormatParser Parser;

    for (auto sampleIdx: xrange(inputVector->Texts.size())) {
        auto& str = inputVector->Texts[sampleIdx];
        try {
            Parser.ParseRecord(str);
        } catch (yexception& e) {
            Cerr << "An error occured while parsing repeated field: " <<
                e.what() << Endl;
            Cerr << "While parsing string: " << str << Endl;
            Cerr << "(sample idx: " << sampleIdx << ")" << Endl;
            ythrow yexception();
        }

        size_t groupSize = Max<size_t>();
        for (auto fieldIdx : xrange(OutputFields.size())) {
            auto& field = OutputFields[fieldIdx];
            size_t pos = Max<size_t>();
            for (auto keyIdx : xrange(Parser.GetKeysCount())) {
                if (Parser.GetKeyAt(keyIdx) == field) {
                    pos = keyIdx;
                    break;
                }
            }
            if (pos == Max<size_t>()) {
                ythrow yexception() << "Repeated fields parser: cannot find field '" << field <<
                    "' in json: " << str << Endl;
            }
            if (groupSize == Max<size_t>()) {
                groupSize = Parser.GetValuesCount(pos);
                (*outputGroups)[0][sampleIdx] = groupSize;
            }
            if (groupSize != Parser.GetValuesCount(pos)) {
                Cerr << "Repeated subfields have different number of items: " << str << Endl;
                Cerr << "Expected " << groupSize << "(field " << OutputFields[0] <<
                    "), but " << field << " has " <<
                    Parser.GetValuesCount(pos) << Endl;
                ythrow yexception();
            }

            outputVectors[fieldIdx]->Texts.reserve(outputVectors[fieldIdx]->Texts.size() + Parser.GetValuesCount(pos));
            for (auto valueIdx : xrange(Parser.GetValuesCount(pos))) {
                outputVectors[fieldIdx]->Texts.push_back(Parser.GetValueAt(pos, valueIdx));
            }
        }
    }
}

template<class FormatParser>
void TRepeatedParserLayer<FormatParser>::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, GroupsSizesName);
    for (auto& field : OutputNames) {
        TryRename(name, newName, field);
    }
}

TMaxPoolingLayer::TMaxPoolingLayer(
    const TString& input, const TString& inputGroupsSizes,
    const TString& output
)
    : Input(input)
    , InputGroupsSizes(inputGroupsSizes)
    , Output(output)
{}

void TMaxPoolingLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["InputGroupsSizes"] = InputGroupsSizes;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TMaxPoolingLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    InputGroupsSizes = fields.at("InputGroupsSizes");
    Output = fields.at("Output");
}

void TMaxPoolingLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TMaxPoolingLayer::Apply(TEvalContext& context) const {
    TMatrix* input = CheckInputAndCast<TMatrix*>(context, Input);
    TGenericMatrix<ui64>* inputGroups = CheckInputAndCast<TGenericMatrix<ui64>*>(context, InputGroupsSizes);
    Y_ENSURE(inputGroups->GetNumRows() == 1);
    TMatrix* output = context.CreateOrGet<TMatrix>(Output);

    ui64 numSamples = inputGroups->GetNumColumns();
    ui64 numColumns = input->GetNumColumns();
    output->Resize(inputGroups->GetNumColumns(), input->GetNumColumns());
    output->FillZero();

    size_t numRows = Accumulate(inputGroups->GetData(), inputGroups->GetData() + inputGroups->GetFlatSize(), 0);
    Y_ENSURE(numRows == input->GetNumRows());

    size_t rowOffset = 0;
    TVector<float> curMax;
    for (size_t sampleIdx : xrange(numSamples)) {
        auto groupSize = (*inputGroups)[0][sampleIdx];
        if (groupSize != 0) {
            curMax.assign((*input)[rowOffset], (*input)[rowOffset] + numColumns);
            for (auto rowIdx : xrange<size_t>(1, groupSize)) {
                const auto& row = (*input)[rowOffset + rowIdx];
                for (auto idx : xrange(numColumns)) {
                    if (row[idx] > curMax[idx]) {
                        curMax[idx] = row[idx];
                    }
                }
            }

            for (auto idx : xrange(numColumns)) {
                (*output)[sampleIdx][idx] = curMax[idx];
            }
        }
        rowOffset += groupSize;
    }
}

void TMaxPoolingLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, InputGroupsSizes);
    TryRename(name, newName, Output);
}

TSumPoolingLayer::TSumPoolingLayer(const TString& input, const TString& inputGroupsSizes, const TString& biases,
    const TString& weights, const TString& output
)
    : Input(input)
    , InputGroupsSizes(inputGroupsSizes)
    , BiasesStr(biases)
    , WeightsStr(weights)
    , Output(output)
{}

void TSumPoolingLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["InputGroupsSizes"] = InputGroupsSizes;
    fields["BiasesStr"] = BiasesStr;
    fields["WeightsStr"] = WeightsStr;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TSumPoolingLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    InputGroupsSizes = fields.at("InputGroupsSizes");
    BiasesStr = fields.at("BiasesStr");
    WeightsStr = fields.at("WeightsStr");
    Output = fields.at("Output");
}

void TSumPoolingLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TSumPoolingLayer::Apply(TEvalContext& context) const {
    TMatrix* input = CheckInputAndCast<TMatrix*>(context, Input);
    TMatrix* biases = CheckInputAndCast<TMatrix*>(context, BiasesStr);
    Y_ENSURE(biases->GetNumColumns() == input->GetNumColumns());
    Y_ENSURE(biases->GetNumRows() == 1);
    TMatrix* weights = nullptr;
    if (WeightsStr) {
        weights = CheckInputAndCast<TMatrix*>(context, WeightsStr);
        Y_ENSURE(weights->GetNumRows() == input->GetNumRows());
        Y_ENSURE(weights->GetNumColumns() == 1);
    }
    TGenericMatrix<ui64>* inputGroups = CheckInputAndCast<TGenericMatrix<ui64>*>(context, InputGroupsSizes);
    Y_ENSURE(inputGroups->GetNumRows() == 1);
    TMatrix* output = context.CreateOrGet<TMatrix>(Output);
    Y_ENSURE(!weights || input->GetNumRows() == weights->GetNumRows());

    ui64 numSamples = inputGroups->GetNumColumns();
    ui64 numColumns = input->GetNumColumns();
    output->Resize(numSamples, numColumns);
    output->FillZero();

    size_t numRows = Accumulate(inputGroups->GetData(), inputGroups->GetData() + inputGroups->GetFlatSize(), 0);
    Y_ENSURE(numRows == input->GetNumRows());

    size_t rowOffset = 0;
    TVector<float> sum;
    for (size_t sampleIdx : xrange(numSamples)) {
        auto groupSize = (*inputGroups)[0][sampleIdx];
        sum.assign((*biases)[0], (*biases)[0] + numColumns);
        for (auto rowIdx : xrange<size_t>(groupSize)) {
            const auto& row = (*input)[rowOffset + rowIdx];
            if (weights) {
                float weight = (*weights)[rowOffset + rowIdx][0];
                for (auto idx : xrange(numColumns)) {
                    sum[idx] += weight * row[idx];
                }
            } else {
                for (auto idx : xrange(numColumns)) {
                    sum[idx] += row[idx];
                }
            }
        }

        for (auto idx : xrange(numColumns)) {
            (*output)[sampleIdx][idx] = sum[idx];
        }
        rowOffset += groupSize;
    }
}

void TSumPoolingLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, InputGroupsSizes);
    TryRename(name, newName, BiasesStr);
    TryRename(name, newName, WeightsStr);
    TryRename(name, newName, Output);
}

TNormalizeRowsLayer::TNormalizeRowsLayer(const TString& input, const TString& output, double normBase)
    : Input(input)
    , Output(output)
    , NormBase(normBase)
{
}

void TNormalizeRowsLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["NormBase"] = SaveToStroka(NormBase);

    ::Save(s, fields);
}

void TNormalizeRowsLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    if (fields.contains("NormBase")) {
        LoadFromStroka(fields["NormBase"], &NormBase);
    } else {
        NormBase = 1e-10;
    }
}

void TNormalizeRowsLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TNormalizeRowsLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        const float norm2 = NormBase + L2NormSquared((*inputMatrix)[row], inputMatrix->GetNumColumns());
        const float normInv = 1.0f / sqrt(norm2);

        std::transform((*inputMatrix)[row], (*inputMatrix)[row] + inputMatrix->GetNumColumns(),
                       (*outputMatrix)[row], [normInv](float x) { return x * normInv; });
    }
}

void TNormalizeRowsLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

// ----------------------------------layernorm----------------------
TLayerNorm::TLayerNorm(const TString& input, const TString& output, bool doBug)
    : Input(input)
    , Output(output)
    , DoBug(doBug)
{
}

void TLayerNorm::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["DoBug"] = SaveToStroka(DoBug);
    ::Save(s, fields);
}

void TLayerNorm::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka(fields.at("DoBug"), &DoBug);
}

void TLayerNorm::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TLayerNorm::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());
    size_t numColumns = inputMatrix->GetNumColumns();
    if (numColumns == 0) {
        return;
    }

    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        double mean = 0.0;
        double norm2 = 1.0;
        const float* inputRow = (*inputMatrix)[row];
        float* outputRow = (*outputMatrix)[row];
        mean = Accumulate(inputRow, inputRow + numColumns, 0.0) * 1.0 / numColumns;
        for (size_t column = 0; column < numColumns; ++column) {
            double value = inputRow[column];
            norm2 += (value - mean) * (value - mean);
        }

        double scaler = 1.0 / sqrt(norm2);
        if (!DoBug) {
            scaler *= sqrt(inputMatrix->GetNumColumns());
        } else {
            scaler *= inputMatrix->GetNumColumns();
        }

        for (size_t column = 0; column < numColumns; ++column) {
            outputRow[column] = (inputRow[column] - mean) * scaler;
        }
    }
}

void TLayerNorm::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}
// --------------------------------------

TLayerNorm2::TLayerNorm2(const TString& input, const TString& output, bool doBug, bool withMean)
    : Input(input)
    , Output(output)
    , DoBug(doBug)
    , WithMean(withMean)
{
    if (doBug) {
        Y_ENSURE(withMean, "doBug should appear with withMean");
    }
}

void TLayerNorm2::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["DoBug"] = SaveToStroka(DoBug);
    fields["WithMean"] = SaveToStroka(WithMean);
    ::Save(s, fields);
}

void TLayerNorm2::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    if (fields.contains("DoBug")) {
        LoadFromStroka(fields.at("DoBug"), &DoBug);
    } else {
        DoBug = DefaultDoBug;
    }
    if (fields.contains("WithMean")) {
        LoadFromStroka(fields.at("WithMean"), &WithMean);
    } else {
        WithMean = true;
    }
}

void TLayerNorm2::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TLayerNorm2::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    size_t numColumns = inputMatrix->GetNumColumns();

    if (numColumns < 2) {
        ythrow yexception() << "TLayerNorm2 input has less then 2 columns";
    }

    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        double mean = 0.0;
        double norm2 = 1.0;
        const float* inputRow = (*inputMatrix)[row];
        float* outputRow = (*outputMatrix)[row];
        mean = Accumulate(inputRow, inputRow + numColumns, 0.0) * 1.0 / numColumns;
        for (size_t column = 0; column < numColumns; ++column) {
            double value = inputRow[column];
            norm2 += (value - mean) * (value - mean);
        }

        // Old bug, we should support such models
        double stdev;
        if (DoBug) {
            stdev = sqrt(norm2) / numColumns;
        } else {
            stdev = sqrt(norm2 / numColumns);
        }

        double scaler = 1.0 / stdev;
        for (size_t column = 0; column < numColumns; ++column) {
            outputRow[column] = (inputRow[column] - mean) * scaler;
        }

        if (WithMean) {
            (*outputMatrix)[row][numColumns - 2] = mean;
        }
        (*outputMatrix)[row][numColumns - 1] = log(stdev);
    }
}

void TLayerNorm2::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

TDotLayer::TDotLayer(const TString& input1, const TString& input2, const TString& output)
    : Input1(input1)
    , Input2(input2)
    , Output(output)
{
}

void TDotLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input1"] = Input1;
    fields["Input2"] = Input2;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TDotLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input1 = fields.at("Input1");
    Input2 = fields.at("Input2");
    Output = fields.at("Output");
}

void TDotLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TDotLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);

    const TMatrix* inputMatrix1 = CheckInputAndCast<TMatrix*>(context, Input1);
    const TMatrix* inputMatrix2 = CheckInputAndCast<TMatrix*>(context, Input2);

    CheckSameColumns(inputMatrix1, inputMatrix2, Input1, Input2);
    const ui32 numRows = Max(inputMatrix1->GetNumRows(), inputMatrix2->GetNumRows());
    if ((inputMatrix1->GetNumRows() != 1) && (inputMatrix2->GetNumRows() != 1)) {
        CheckSameRows(inputMatrix1, inputMatrix2, Input1, Input2);
    }
    outputMatrix->Resize(numRows, 1);

    for (size_t row = 0; row < numRows; ++row) {
        float dot;
        if ((inputMatrix1->GetNumRows() == 1) && (inputMatrix2->GetNumRows() != 1)) {
            dot  = DotProduct((*inputMatrix1)[0], (*inputMatrix2)[row], inputMatrix1->GetNumColumns());
        } else if ((inputMatrix1->GetNumRows() != 1) && (inputMatrix2->GetNumRows() == 1)) {
            dot  = DotProduct((*inputMatrix1)[row], (*inputMatrix2)[0], inputMatrix1->GetNumColumns());
        } else {
            dot  = DotProduct((*inputMatrix1)[row], (*inputMatrix2)[row], inputMatrix1->GetNumColumns());
        }
        (*outputMatrix)[row][0] = dot;
    }
}

void TDotLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input1);
    TryRename(name, newName, Input2);
    TryRename(name, newName, Output);
}

TConcatLayer::TConcatLayer(const TVector<TString>& inputs, const TString& output)
    : Inputs(inputs)
    , Output(output)
{
}

void TConcatLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Inputs"] = SaveToStroka(Inputs);
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TConcatLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    LoadFromStroka(fields["Inputs"], &Inputs);
    Output = fields.at("Output");
}

void TConcatLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TConcatLayer::RenameVariable(const TString& name, const TString& newName) {
    for (size_t i = 0; i < Inputs.size(); ++i) {
        TryRename(name, newName, Inputs[i]);
    }
    TryRename(name, newName, Output);
}

void TConcatLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);

    TVector<const TMatrix*> inputMatrices(Reserve(Inputs.size()));
    for (auto& input : Inputs) {
        inputMatrices.push_back(CheckInputAndCast<const TMatrix*>(context, input));
    }

    size_t outputSize = 0;
    size_t numRows = 0;
    size_t nonConstMatrixInd = 0;
    for (size_t i = 0; i < inputMatrices.size(); ++i) {
        numRows = Max(numRows, inputMatrices[i]->GetNumRows());
        if (inputMatrices[i]->GetNumRows() != 1) {
            nonConstMatrixInd = i;
        }
    }
    for (size_t i = 0; i < inputMatrices.size(); ++i) {
        if (inputMatrices[i]->GetNumRows() != 1) {
            CheckSameRows(inputMatrices[nonConstMatrixInd], inputMatrices[i],
                Inputs[nonConstMatrixInd], Inputs[i]);
        }
        outputSize += inputMatrices[i]->GetNumColumns();
    }

    outputMatrix->Resize(numRows, outputSize);

    for (size_t row = 0; row < outputMatrix->GetNumRows(); ++row) {
        size_t columnOffset = 0;
        for (const TMatrix* m : inputMatrices) {
            if (m->GetNumRows() == 1) {
                std::copy_n((*m)[0], m->GetNumColumns(), (*outputMatrix)[row] + columnOffset);
            } else {
                std::copy_n((*m)[row], m->GetNumColumns(), (*outputMatrix)[row] + columnOffset);
            }
            columnOffset += m->GetNumColumns();
        }
    }
}

TAddLayer::TAddLayer(const TVector<TString>& inputs, const TString& output)
    : Inputs(inputs)
    , Output(output)
{
    Y_ENSURE(Inputs);
}

void TAddLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Inputs"] = SaveToStroka(Inputs);
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TAddLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    if (fields.contains("Input1")) {
        Inputs.clear();
        Inputs.push_back(fields.at("Input1"));
        Inputs.push_back(fields.at("Input2"));
    } else {
        LoadFromStroka(fields.at("Inputs"), &Inputs);
    }
    Output = fields.at("Output");
}

void TAddLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TAddLayer::RenameVariable(const TString& name, const TString& newName) {
    for (auto& var : Inputs) {
        TryRename(name, newName, var);
    }
    TryRename(name, newName, Output);

    for (const auto& var : Inputs) {
        Y_ENSURE(var != Output);
    }
}

void TAddLayer::Apply(TEvalContext& context) const {
    Y_ENSURE(Inputs);

    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);

    const TMatrix* inputMatrix1 = CheckInputAndCast<TMatrix*>(context, Inputs[0]);
    outputMatrix->Resize(inputMatrix1->GetNumRows(), inputMatrix1->GetNumColumns());
    outputMatrix->FillZero();

    for (const auto& input : Inputs) {
        const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, input);
        CheckSameSize(outputMatrix, inputMatrix, Inputs[0], input);

        float* outputData = outputMatrix->GetData();
        const float* inputData = inputMatrix->GetData();
        for (auto i : xrange(outputMatrix->GetFlatSize())) {
            outputData[i] += inputData[i];
        }
    }
}

TWeightedAddLayer::TWeightedAddLayer(const TVector<TString>& inputs, const TString& weights, const TString& output)
    : Inputs(inputs)
    , Weights(weights)
    , Output(output)
{
    Y_ENSURE(Inputs);
}

void TWeightedAddLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Inputs"] = SaveToStroka(Inputs);
    fields["Weights"] = Weights;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TWeightedAddLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    LoadFromStroka(fields.at("Inputs"), &Inputs);
    Weights = fields.at("Weights");
    Output = fields.at("Output");
}

void TWeightedAddLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TWeightedAddLayer::RenameVariable(const TString& name, const TString& newName) {
    for (auto& var : Inputs) {
        TryRename(name, newName, var);
    }
    TryRename(name, newName, Output);
    TryRename(name, newName, Weights);

    for (const auto& var : Inputs) {
        Y_ENSURE(var != Output);
    }
    Y_ENSURE(Output != Weights);
}

void TWeightedAddLayer::Apply(TEvalContext& context) const {
    Y_ENSURE(Inputs);

    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);

    const TMatrix* inputMatrix1 = CheckInputAndCast<TMatrix*>(context, Inputs[0]);
    outputMatrix->Resize(inputMatrix1->GetNumRows(), inputMatrix1->GetNumColumns());
    outputMatrix->FillZero();

    const TMatrix* weightsMatrix = CheckInputAndCast<TMatrix*>(context, Weights);
    Y_ENSURE(weightsMatrix->GetSize() == Inputs.size());

    size_t inputIdx = 0;
    for (const auto& input : Inputs) {
        const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, input);
        CheckSameSize(outputMatrix, inputMatrix, Inputs[0], input);

        float* outputData = outputMatrix->GetData();
        float weight = weightsMatrix->GetData()[inputIdx];
        const float* inputData = inputMatrix->GetData();
        for (auto i : xrange(outputMatrix->GetFlatSize())) {
            outputData[i] += weight * inputData[i];
        }
        ++inputIdx;
    }
}

TMultiCosLayer::TMultiCosLayer(const TString& input1, const TString& input2,
    const TString& weightMatrixInput, const TString& biasMatrixInput,
    const TString& output, float base)
    : Input1(input1)
    , Input2(input2)
    , WeightMatrixInput(weightMatrixInput)
    , BiasMatrixInput(biasMatrixInput)
    , Output(output)
    , Base(base)
{
}

void TMultiCosLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input1"] = Input1;
    fields["Input2"] = Input2;
    fields["WeightMatrixInput"] = WeightMatrixInput;
    fields["BiasMatrixInput"] = BiasMatrixInput;
    fields["Output"] = Output;
    fields["Base"] = SaveToStroka(Base);
    ::Save(s, fields);
}

void TMultiCosLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input1 = fields.at("Input1");
    Input2 = fields.at("Input2");
    WeightMatrixInput = fields.at("WeightMatrixInput");
    BiasMatrixInput = fields.at("BiasMatrixInput");
    Output = fields.at("Output");
    if (fields.contains("Base")) {
        LoadFromStroka(fields.at("Base"), &Base);
    } else {
        Base = 0;
    }
}

void TMultiCosLayer::Init(const TContext& context) {
    WeightMatrix = CheckInputAndCast<TMatrix*>(context, WeightMatrixInput);
    BiasMatrix = CheckInputAndCast<TMatrix*>(context, BiasMatrixInput);
}

void TMultiCosLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);

    const TMatrix* inputMatrix1 = CheckInputAndCast<TMatrix*>(context, Input1);
    const TMatrix* inputMatrix2 = CheckInputAndCast<TMatrix*>(context, Input2);

    CheckSameSize(inputMatrix1, inputMatrix2, Input1, Input2);
    CheckSameSize(WeightMatrix, BiasMatrix, WeightMatrixInput, BiasMatrixInput);

    CheckSameColumns(inputMatrix1, WeightMatrix, Input1, WeightMatrixInput);

    outputMatrix->Resize(inputMatrix1->GetNumRows(), WeightMatrix->GetNumRows());

    TVector<float> buffer1;
    TVector<float> buffer2;

    buffer1.yresize(inputMatrix1->GetNumColumns());
    buffer2.yresize(inputMatrix1->GetNumColumns());

    // TODO(agusakov): Speed this up if needed.
    for (size_t row = 0; row < inputMatrix1->GetNumRows(); ++row) {
        for (size_t dim = 0; dim < WeightMatrix->GetNumRows(); ++dim) {
            const float* bias = (*BiasMatrix)[dim];
            const float* weight = (*WeightMatrix)[dim];
            for (size_t column = 0; column < inputMatrix1->GetNumColumns(); ++column) {
                buffer1[column] = ((*inputMatrix1)[row][column] + bias[column]) * weight[column];
            }
            for (size_t column = 0; column < inputMatrix1->GetNumColumns(); ++column) {
                buffer2[column] = ((*inputMatrix2)[row][column] + bias[column]) * weight[column];
            }
            const float currentDot = DotProduct(buffer1.data(), buffer2.data(), buffer1.size());
            const float squaredNorm1 = Base + L2NormSquared(buffer1.data(), buffer1.size());
            const float squaredNorm2 = Base + L2NormSquared(buffer2.data(), buffer2.size());
            (*outputMatrix)[row][dim] = currentDot / sqrt(squaredNorm1) / sqrt(squaredNorm2);
        }
    }
}

void TMultiCosLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input1);
    TryRename(name, newName, Input2);
    TryRename(name, newName, WeightMatrixInput);
    TryRename(name, newName, BiasMatrixInput);
    TryRename(name, newName, Output);
}

TAffineLayer::TAffineLayer(const TString& input, const TString& transformMatrixInput,
    const TString& biasMatrixInput, const TString& output)
    : Input(input)
    , TransfromMatrixInput(transformMatrixInput)
    , BiasMatrixInput(biasMatrixInput)
    , Output(output)
{
}

void TAffineLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["TransfromMatrixInput"] = TransfromMatrixInput;
    fields["BiasMatrixInput"] = BiasMatrixInput;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TAffineLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    TransfromMatrixInput = fields.at("TransfromMatrixInput");
    BiasMatrixInput = fields.at("BiasMatrixInput");
    Output = fields.at("Output");
}

void TAffineLayer::Init(const TContext& context) {
    TransfromMatrix = CheckInputAndCast<TMatrix*>(context, TransfromMatrixInput);
    BiasMatrix = CheckInputAndCast<TMatrix*>(context, BiasMatrixInput);
}

void TAffineLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);

    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    CheckSameColumns(inputMatrix, TransfromMatrix, Input, TransfromMatrixInput);

    Y_ASSERT(BiasMatrix->GetNumRows() == 1);
    Y_ASSERT(BiasMatrix->GetNumColumns() == TransfromMatrix->GetNumRows());

    outputMatrix->Resize(inputMatrix->GetNumRows(), TransfromMatrix->GetNumRows());

    MatrixProduct(*inputMatrix, *TransfromMatrix, *BiasMatrix, outputMatrix);
}

void TAffineLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, TransfromMatrixInput);
    TryRename(name, newName, BiasMatrixInput);
    TryRename(name, newName, Output);
}

TScaleLayer::TScaleLayer(const TString& input, const TString& multiplier,
    const TString& bias, const TString& output)
    : Input(input)
    , Multiplier(multiplier)
    , Bias(bias)
    , Output(output)
{}

void TScaleLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Multiplier"] = Multiplier;
    fields["Bias"] = Bias;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TScaleLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Multiplier = fields["Multiplier"];
    Bias = fields["Bias"];
    Output = fields.at("Output");
}

void TScaleLayer::Init(const TContext& context) {
    TMatrix* multiplierMatrix = CheckInputAndCast<TMatrix*>(context, Multiplier);
    MultiplierScalar = (*multiplierMatrix)[0];
    TMatrix* biasMatrix = CheckInputAndCast<TMatrix*>(context, Bias);
    BiasScalar = (*biasMatrix)[0];
}

void TScaleLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    // TODO(agusakov): Speed this up.
    float biasScalar = BiasScalar ? *BiasScalar : 0;
    float multiplierScalar = *MultiplierScalar;
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t column = 0; column < inputMatrix->GetNumColumns(); ++column) {
            (*outputMatrix)[row][column] = (*inputMatrix)[row][column] *
                multiplierScalar + biasScalar;
        }
    }
}

void TScaleLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Multiplier);
    TryRename(name, newName, Bias);
    TryRename(name, newName, Output);
}

TPreDotScaleLayer::TPreDotScaleLayer(const TString& input, const TString& multiplier,
    const TString& bias, const TString& output)
    : Input(input)
    , Multiplier(multiplier)
    , Bias(bias)
    , Output(output)
{}

TPreDotScaleLayer::TPreDotScaleLayer(const TString& input, const TString& multiplier,
    const TString& bias, float biasMultiplier, const TString& output)
    : Input(input)
    , Multiplier(multiplier)
    , Bias(bias)
    , Output(output)
    , BiasMultiplier(biasMultiplier)
{}

void TPreDotScaleLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Multiplier"] = Multiplier;
    fields["Bias"] = Bias;
    fields["Output"] = Output;
    fields["BiasMultiplier"] = SaveToStroka(BiasMultiplier);
    ::Save(s, fields);
}

void TPreDotScaleLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Multiplier = fields["Multiplier"];
    Bias = fields["Bias"];
    Output = fields.at("Output");
    if (fields.contains("BiasMultiplier")) {
        LoadFromStroka(fields.at("BiasMultiplier"), &BiasMultiplier);
    } else {
        BiasMultiplier = 1.0;
    }
}

void TPreDotScaleLayer::Init(const TContext&) {
}

void TPreDotScaleLayer::Apply(TEvalContext& context) const {
    const TMatrix& input = *CheckInputAndCast<TMatrix*>(context, Input);
    TMatrix& result = *context.CreateOrGet<TMatrix>(Output);
    result.Resize(input.GetNumRows(), input.GetNumColumns() + 1);

    float bias = 1.0;
    if (!Bias.empty()) {
        const TMatrix* m = CheckInputAndCast<TMatrix*>(context, Bias);
        Y_ENSURE(m != nullptr);
        Y_ENSURE(m->GetNumRows() == 1 && m->GetNumColumns() == 1);
        bias = (*m)[0][0];
    }
    bias *= BiasMultiplier;
    float scale = 1.0;
    if (!Multiplier.empty()) {
        const TMatrix* m = CheckInputAndCast<TMatrix*>(context, Multiplier);
        Y_ENSURE(m != nullptr);
        Y_ENSURE(m->GetNumRows() == 1 && m->GetNumColumns() == 1);
        scale = (*m)[0][0];
    }

    for (auto i = 0u; i != input.GetNumRows(); ++i) {
        result[i][0] = bias;
        Transform(input[i], input[i] + input.GetNumColumns(), result[i] + 1,
                  [scale](float v) { return v * scale; });
    }
}

TVector<TString> TPreDotScaleLayer::GetInputs() const {
    TVector<TString> inputs{Input};
    if (!Multiplier.empty()) {
        inputs.push_back(Multiplier);
    }
    if (!Bias.empty()) {
        inputs.push_back(Bias);
    }
    return inputs;
}

void TPreDotScaleLayer::RenameVariable(const TString& name,
                                       const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Multiplier);
    TryRename(name, newName, Bias);
    TryRename(name, newName, Output);
}

TShallowCopyLayer::TShallowCopyLayer(const TString& input, const TString& output)
    : Input(input)
    , Output(output)
{}

void TShallowCopyLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TShallowCopyLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
}

void TShallowCopyLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TShallowCopyLayer::Apply(TEvalContext& context) const {
    context[Output] = context.at(Input);
}

void TShallowCopyLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

TAddRowLayer::TAddRowLayer(const TString& input,const TString& addVectorStr,
        const TString& output)
    : Input(input)
    , AddVectorStr(addVectorStr)
    , Output(output)
{
}

void TAddRowLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["AddVectorStr"] = AddVectorStr;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TAddRowLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    AddVectorStr = fields.at("AddVectorStr");
    Output = fields.at("Output");
}

void TAddRowLayer::Init(const TContext& context) {
    AddVector = CheckInputAndCast<TMatrix*>(context, AddVectorStr);
}

void TAddRowLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    CheckSameColumns(inputMatrix, AddVector, Input, AddVectorStr);

    Y_ASSERT(AddVector->GetNumRows() == 1);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    // TODO(agusakov): Speed this up.
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t column = 0; column < inputMatrix->GetNumColumns(); ++column) {
            (*outputMatrix)[row][column] =
                (*inputMatrix)[row][column] + (*AddVector)[0][column];
        }
    }
}

void TAddRowLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, AddVectorStr);
    TryRename(name, newName, Output);
}

TOuterScaleLayer::TOuterScaleLayer(const TString& input, const TString& alpha, const TString& beta,
    const TString& output)
    : Input(input)
    , Alpha(alpha)
    , Beta(beta)
    , Output(output)
{
}

void TOuterScaleLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Alpha"] = Alpha;
    fields["Beta"] = Beta;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TOuterScaleLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Alpha = fields.at("Alpha");
    Beta = fields.at("Beta");
    Output = fields.at("Output");
}

void TOuterScaleLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TOuterScaleLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);
    const TMatrix* alphaMatrix = CheckInputAndCast<TMatrix*>(context, Alpha);
    const TMatrix* betaMatrix = CheckInputAndCast<TMatrix*>(context, Beta);

    Y_ENSURE(alphaMatrix->GetNumRows() == 1);
    Y_ENSURE(betaMatrix->GetNumRows() == 1);
    Y_ENSURE(alphaMatrix->GetNumColumns() == betaMatrix->GetNumColumns());
    Y_ENSURE(inputMatrix->GetNumColumns() == 1);

    outputMatrix->Resize(inputMatrix->GetNumRows(), alphaMatrix->GetNumColumns());

    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t column = 0; column < alphaMatrix->GetNumColumns(); ++column) {
            (*outputMatrix)[row][column] = (*alphaMatrix)[0][column] * (*inputMatrix)[row][0] +
                (*betaMatrix)[0][column];
        }
    }
}

void TOuterScaleLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Alpha);
    TryRename(name, newName, Beta);
    TryRename(name, newName, Output);
}

TSparseRescalerLayer::TSparseRescalerLayer(const TString& input, const TString& parameters,
    const TString& output)
    : Input(input)
    , Parameters(parameters)
    , Output(output)
{
}

void TSparseRescalerLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Parameters"] = Parameters;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TSparseRescalerLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Parameters = fields.at("Parameters");
    Output = fields.at("Output");
}

void TSparseRescalerLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TSparseRescalerLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TSparseMatrix();
    }

    TSparseMatrix* input = CheckInputAndCast<TSparseMatrix*>(context, Input);
    TSparseMatrix* output = CheckInputAndCast<TSparseMatrix*>(context, Output);
    const TMatrix& params = *CheckInputAndCast<TMatrix*>(context, Parameters);

    Y_ENSURE(params.GetNumRows() == 1);
    Y_ENSURE(params.GetNumColumns() == 4);

    output->Vectors.resize(input->Vectors.size());

    const float* p = params.GetData();
    for (auto rowIdx: xrange(input->Vectors.size())) {
        output->Vectors[rowIdx] = input->Vectors[rowIdx];
        for (auto& val : output->Vectors[rowIdx].Values) {
            val = p[0] + p[1] * val + p[2] * sqrt(val + 0.01) +
                p[3] * log(val + 1);
        }
    }
}

void TSparseRescalerLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Parameters);
    TryRename(name, newName, Output);
}

TMulRowLayer::TMulRowLayer(const TString& input, const TString& addVectorStr,
    const TString& output)
    : Input(input)
    , MulVectorStr(addVectorStr)
    , Output(output)
{}

void TMulRowLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["MulVectorStr"] = MulVectorStr;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TMulRowLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    MulVectorStr = fields.at("MulVectorStr");
    Output = fields.at("Output");
}

void TMulRowLayer::Init(const TContext& context) {
    MulVector = CheckInputAndCast<TMatrix*>(context, MulVectorStr);
}

void TMulRowLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    CheckSameColumns(inputMatrix, MulVector, Input, MulVectorStr);

    Y_ASSERT(MulVector->GetNumRows() == 1);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    // TODO(agusakov): Speed this up.
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t column = 0; column < inputMatrix->GetNumColumns(); ++column) {
            (*outputMatrix)[row][column] =
                (*inputMatrix)[row][column] * (*MulVector)[0][column];
        }
    }
}

void TMulRowLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, MulVectorStr);
    TryRename(name, newName, Output);
}

TAddMulLayer::TAddMulLayer(const TString& input, const TString& addVectorInput,
    const TString& mulVectorInput, const TString& output)
    : Input(input)
    , AddVectorInput(addVectorInput)
    , MulVectorInput(mulVectorInput)
    , Output(output)
{
}

void TAddMulLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["AddVectorInput"] = AddVectorInput;
    fields["MulVectorInput"] = MulVectorInput;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TAddMulLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    AddVectorInput = fields.at("AddVectorInput");
    MulVectorInput = fields.at("MulVectorInput");
    Output = fields.at("Output");
}

void TAddMulLayer::Init(const TContext& context) {
    AddVector = CheckInputAndCast<TMatrix*>(context, AddVectorInput);
    MulVector = CheckInputAndCast<TMatrix*>(context, MulVectorInput);
}

void TAddMulLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    CheckSameSize(AddVector, MulVector, AddVectorInput, MulVectorInput);
    CheckSameColumns(inputMatrix, MulVector, Input, MulVectorInput);

    Y_ASSERT(MulVector->GetNumRows() == 1);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    // TODO(agusakov): Speed this up.
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t column = 0; column < inputMatrix->GetNumColumns(); ++column) {
            (*outputMatrix)[row][column] =
                ((*inputMatrix)[row][column] + (*AddVector)[0][column])
                * (*MulVector)[0][column];
        }
    }
}

void TAddMulLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, MulVectorInput);
    TryRename(name, newName, AddVectorInput);
    TryRename(name, newName, Output);
}

TMulAddLayer::TMulAddLayer(const TString& input, const TString& mulVectorInput,
    const TString& addVectorInput, const TString& output)
    : Input(input)
    , MulVectorInput(mulVectorInput)
    , AddVectorInput(addVectorInput)
    , Output(output)
{
}

void TMulAddLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["MulVectorInput"] = MulVectorInput;
    fields["AddVectorInput"] = AddVectorInput;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TMulAddLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    MulVectorInput = fields.at("MulVectorInput");
    AddVectorInput = fields.at("AddVectorInput");
    Output = fields.at("Output");
}

void TMulAddLayer::Init(const TContext& context) {
    MulVector = CheckInputAndCast<TMatrix*>(context, MulVectorInput);
    AddVector = CheckInputAndCast<TMatrix*>(context, AddVectorInput);
}

void TMulAddLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    CheckSameSize(AddVector, MulVector, AddVectorInput, MulVectorInput);
    CheckSameColumns(inputMatrix, MulVector, Input, MulVectorInput);

    Y_ASSERT(MulVector->GetNumRows() == 1);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    // TODO(mbusel): Speed this up.
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t column = 0; column < inputMatrix->GetNumColumns(); ++column) {
            (*outputMatrix)[row][column] =
                (*inputMatrix)[row][column] * (*MulVector)[0][column]
                + (*AddVector)[0][column];
        }
    }
}

void TMulAddLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, MulVectorInput);
    TryRename(name, newName, AddVectorInput);
    TryRename(name, newName, Output);
}

TSoftSignLayer::TSoftSignLayer()
    : Constant(1.0)
    , Scale(0.5)
    , Bias(0.5)
{
}

TSoftSignLayer::TSoftSignLayer(const TString& input, const TString& output, float constant,
    float scale, float bias)
    : Input(input)
    , Output(output)
    , Constant(constant)
    , Scale(scale)
    , Bias(bias)
{
}

void TSoftSignLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["Constant"] = SaveToStroka(Constant);
    fields["Scale"] = SaveToStroka(Scale);
    fields["Bias"] = SaveToStroka(Bias);
    ::Save(s, fields);
}

void TSoftSignLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka<float>(fields.at("Constant"), &Constant);
    LoadFromStroka<float>(fields.at("Scale"), &Scale);
    LoadFromStroka<float>(fields.at("Bias"), &Bias);
}

void TSoftSignLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

float TSoftSignLayer::SoftSign(float x) const {
    return Bias + Scale * x / (Constant + fabs(x));
}

void TSoftSignLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t col = 0; col < inputMatrix->GetNumColumns(); ++col) {
            (*outputMatrix)[row][col] = SoftSign((*inputMatrix)[row][col]);
        }
    }
}

void TSoftSignLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

// -------------------------remap-------------------------------

TRemapLayer::TRemapLayer() {}

TRemapLayer::TRemapLayer(const TString& input, const TString& output, TVector<TVector<float>> remapFrom, TVector<TVector<float>> remapTo)
    : Input(input)
    , Output(output)
    , RemapFrom(std::move(remapFrom))
    , RemapTo(std::move(remapTo))
{
    Y_VERIFY(RemapFrom.size() == RemapTo.size());
    for (ui64 i = 0; i < RemapFrom.size(); ++i) {
        Y_VERIFY(RemapFrom[i].size() == RemapTo[i].size());
        Y_VERIFY(RemapFrom[i].size() != 1); // empty for no remap

        for (ui64 j = 1; j < RemapFrom[i].size(); ++j) {
            Y_VERIFY(RemapFrom[i][j] >= RemapFrom[i][j - 1]);
            Y_VERIFY(RemapTo[i][j] >= RemapTo[i][j - 1]);
        }
    }
}

float TRemapLayer::Map(float x, ui64 idx) const {
    const TVector<float>& remapFrom = RemapFrom[idx];
    const TVector<float>& remapTo = RemapTo[idx];

    if (!remapFrom.size()) {
        return x;
    }

    ui64 idx2;
    if (x <= remapFrom[0]) {
        idx2 = 1;
    } else if (x >= remapFrom.back()) {
        idx2 = remapTo.size() - 1;
    } else {
        idx2 = UpperBound(remapFrom.begin(), remapFrom.end(), x) - remapFrom.begin();
    }
    ui64 idx1 = idx2 - 1;
    return remapTo[idx1] + (remapTo[idx2] - remapTo[idx1]) * (x - remapFrom[idx1]) / (remapFrom[idx2] - remapFrom[idx1]);
}

void TRemapLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["RemapFrom"] = SaveToStroka(RemapFrom);
    fields["RemapTo"] = SaveToStroka(RemapTo);
    ::Save(s, fields);
}

void TRemapLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka<TVector<TVector<float>>>(fields.at("RemapFrom"), &RemapFrom);
    LoadFromStroka<TVector<TVector<float>>>(fields.at("RemapTo"), &RemapTo);
}

void TRemapLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TRemapLayer::Apply(TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t col = 0; col < inputMatrix->GetNumColumns(); ++col) {
            (*outputMatrix)[row][col] = Map((*inputMatrix)[row][col], col);
        }
    }
}

void TRemapLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

// ------------------------zero--------------------------------

TZeroLayer::TZeroLayer() {}

TZeroLayer::TZeroLayer(const TString& input, const TString& output)
    : Input(input)
    , Output(output)
{}

void TZeroLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TZeroLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
}

void TZeroLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TZeroLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TSamplesVector* inputSamplesVector = CheckInputAndCast<TSamplesVector*>(context, Input);
    const TVector<TAtomicSharedPtr<ISample>>& samples = inputSamplesVector->GetSamples();

    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);

    outputMatrix->Resize(samples.size(), 1);
    for (size_t row = 0; row < samples.size(); ++row) {
        (*outputMatrix)[row][0] = 0;
    }
}

void TZeroLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

// ---------------------group softmax---------------------------
TGroupSoftmaxLayer::TGroupSoftmaxLayer()
    : GroupSize(0)
{
}

TGroupSoftmaxLayer::TGroupSoftmaxLayer(const TString& input, const TString& output, size_t groupSize)
    : Input(input)
    , Output(output)
    , GroupSize(groupSize)
{}

void TGroupSoftmaxLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["GroupSize"] = SaveToStroka(GroupSize);
    ::Save(s, fields);
}

void TGroupSoftmaxLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka(fields.at("GroupSize"), &GroupSize);
}

void TGroupSoftmaxLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void Softmax(const float* input, float* output, size_t N) {
    float partition = 0;
    float maxValue = *MaxElement(input, input + N);
    for (auto i : xrange(N)) {
        output[i] = exp(input[i] - maxValue);
        partition += output[i];
    }
    double c = 1.0 / partition;
    for (auto i : xrange(N)) {
        output[i] *= c;
    }
}

void TGroupSoftmaxLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());
    const float* inputPtr = inputMatrix->GetData();
    float* outputPtr = outputMatrix->GetData();
    for (size_t i = 0; i < inputMatrix->GetSize(); i += GroupSize) {
        Softmax(inputPtr + i, outputPtr + i, GroupSize);
    }
}

void TGroupSoftmaxLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

// ---------------------quantize---------------------------
TQuantizeLayer::TQuantizeLayer()
    : NumBins(0)
{
}

TQuantizeLayer::TQuantizeLayer(const TString& input, const TString& output, size_t numBins)
    : Input(input)
    , Output(output)
    , NumBins(numBins)
{}

void TQuantizeLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["NumBins"] = SaveToStroka(NumBins);
    ::Save(s, fields);
}

void TQuantizeLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka(fields.at("NumBins"), &NumBins);
}

void TQuantizeLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TQuantizeLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    const float* inputPtr = inputMatrix->GetData();
    float* outputPtr = outputMatrix->GetData();
    float coeff = 2.0 / (NumBins - 1);
    for (size_t i = 0; i < inputMatrix->GetSize(); ++i) {
        // We get input in range -1..1, and should output a quantized value in range [-1,1]
        float val = 0.5 * (1 + inputPtr[i]) * (NumBins - 1);
        size_t bin = round(val);
        outputPtr[i] = -1 + bin * coeff;
    }
}

void TQuantizeLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

// ---------------------compressor---------------------------
namespace {
    bool IsPowerOfTwo (size_t x) {
        return ((x != 0) && !(x & (x - 1)));
    }

    size_t GetMaxBit(size_t x) {
        size_t res = 0;
        while (x > 0) {
            x >>= 1;
            ++res;
        }
        return res;
    }
}

TCompressorLayer::TCompressorLayer()
    : NumBins(0)
    , NumBits(0)
    , Min(-1)
    , Max(1)
{
}

TCompressorLayer::TCompressorLayer(const TString& input, const TString& output, size_t numBins,
        double min, double max, size_t numBits)
    : Input(input)
    , Output(output)
    , NumBins(numBins)
    , NumBits(numBits)
    , Min(min)
    , Max(max)
{
    if (NumBits == 0) {
        NumBits = GetMaxBit(NumBins - 1);
    }

    if (NumBits > 8) {
        throw yexception() << "NumBits > 8 are not supported" << Endl;
    }

    if (pow(2, NumBits) < NumBins) {
        throw yexception() << "Number of bins " << NumBins << " is greater than representable by " << NumBits << " bits" << Endl;
    }

    if (Max <= Min) {
        throw yexception() << "Max should be greater than Min" << Endl;
    }
}

void TCompressorLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["NumBins"] = SaveToStroka(NumBins);
    fields["NumBits"] = SaveToStroka(NumBits);
    fields["Max"] = SaveToStroka(Max);
    fields["Min"] = SaveToStroka(Min);
    ::Save(s, fields);
}

void TCompressorLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka(fields.at("NumBins"), &NumBins);
    LoadFromStroka(fields.at("Max"), &Max);
    LoadFromStroka(fields.at("Min"), &Min);
    if (fields.contains("NumBits")) {
        LoadFromStroka(fields.at("NumBits"), &NumBits);
    } else {
        NumBits = GetMaxBit(NumBins - 1);
    }
}

void TCompressorLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TBitVectorWriter::Write(ui8 val, size_t numBits) {
    if (Offset >= 8) {
        ++Data;
        Offset -= 8;
    }

    if (Offset + numBits > 8) {
        size_t numLowBits = Offset + numBits - 8;
        // remove high bits
        ui8 val2 = val & ((1 << numBits) - 1);

        ui8 upperBits = val2 >> numLowBits;
        *Data += upperBits;
        Data++;
        ui8 lowerBits = val2 << (8 - numLowBits);
        *Data = lowerBits;
        Offset = numLowBits;
    } else {
        ui8 val2 = val << (8 - numBits);
        *Data += val2 >> Offset;
        Offset += numBits;
    }
}

ui8 TBitVectorReader::Get(size_t numBits) {
    if (Offset >= 8) {
        ++Data;
        Offset -= 8;
    }

    ui8 res;
    if (Offset + numBits > 8) {
        size_t highMask = (1 << (8 - Offset)) - 1;
        size_t numLowBits = Offset + numBits - 8;
        res = ((*Data & highMask) << numLowBits) + (*(Data + 1) >> (8 - numLowBits));
        ++Data;
        Offset = numLowBits;
    } else {
        res = static_cast<ui8>(*Data << Offset) >> (8 - numBits);
        Offset += numBits;
    }
    return res;
}

void TCompressorLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TCharMatrix();
    }
    TCharMatrix* outputMatrix = CheckInputAndCast<TCharMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);
    size_t bitsPerInput = NumBits;
    size_t outputNumBits = bitsPerInput * inputMatrix->GetNumColumns();
    size_t outputNumChars = outputNumBits / 8 + (outputNumBits % 8 > 0);

    outputMatrix->Resize(inputMatrix->GetNumRows(), outputNumChars);
    outputMatrix->SetNumUnpackedColumns(inputMatrix->GetNumColumns());
    memset(outputMatrix->GetData(), 0, outputMatrix->GetSize());
    if (outputNumChars == 0) {
        return;
    }

    float maxBin = NumBins - 1;
    float invlen = 1.0 / (Max - Min);
    const float* inputPtr = inputMatrix->GetData();
    ui8* outputPtr = outputMatrix->GetData();
    for (size_t rowIdx = 0; rowIdx < inputMatrix->GetNumRows(); ++rowIdx) {
        const float* inputRow = inputPtr + rowIdx * inputMatrix->GetNumColumns();
        ui8* outputRow = outputPtr + rowIdx * outputNumChars;

        TBitVectorWriter bits(outputRow);
        for (size_t colIdx = 0; colIdx < inputMatrix->GetNumColumns(); ++colIdx) {
            // convert input into range [0, NumBins - 1]
            float val = invlen * (inputRow[colIdx] - Min) * (NumBins - 1);
            ui8 bin = (ui8)round(::Min(::Max(val, 0.0f), maxBin));
            bits.Write(bin, bitsPerInput);
        }
        Y_ASSERT(bits.GetData() == (outputRow + outputNumChars - 1));
    }
}

void TCompressorLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

// ---------------------decompressor---------------------------
TDecompressorLayer::TDecompressorLayer()
    : NumBins(0)
    , NumBits(0)
    , Min(-1)
    , Max(1)
{}

TDecompressorLayer::TDecompressorLayer(const TString& input, const TString& output, size_t numBins,
        double min, double max, size_t numBits)
    : Input(input)
    , Output(output)
    , NumBins(numBins)
    , NumBits(numBits)
    , Min(min)
    , Max(max)
{
    if (NumBits == 0) {
        NumBits = GetMaxBit(NumBins - 1);
    }

    if (NumBits > 8) {
        throw yexception() << "NumBits > 8 are not supported" << Endl;
    }

    if (pow(2, NumBits) < NumBins) {
        throw yexception() << "Number of bins " << NumBins << " is greater than representable by " << NumBits << " bits" << Endl;
    }

    if (Max <= Min) {
        throw yexception() << "Max should be greater than Min" << Endl;
    }
}

void TDecompressorLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["NumBins"] = SaveToStroka(NumBins);
    fields["Max"] = SaveToStroka(Max);
    fields["Min"] = SaveToStroka(Min);
    fields["NumBits"] = SaveToStroka(NumBits);
    ::Save(s, fields);
}

void TDecompressorLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka(fields.at("NumBins"), &NumBins);
    LoadFromStroka(fields.at("Max"), &Max);
    LoadFromStroka(fields.at("Min"), &Min);
    if (fields.contains("NumBits")) {
        LoadFromStroka(fields.at("NumBits"), &NumBits);
    } else {
        NumBits = GetMaxBit(NumBins - 1);
    }
}

void TDecompressorLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TDecompressorLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TCharMatrix* inputMatrix = CheckInputAndCast<TCharMatrix*>(context, Input);
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumUnpackedColumns());
    memset(outputMatrix->GetData(), 0, sizeof(float) * outputMatrix->GetSize());
    ui8* inputPtr = inputMatrix->GetData();
    float* outputPtr = outputMatrix->GetData();
    if (outputMatrix->GetSize() == 0) {
        return;
    }

    size_t bitsPerInput = NumBits;
    const float coeff = CalcCoef(Min, Max, NumBins);
    for (size_t rowIdx = 0; rowIdx < inputMatrix->GetNumRows(); ++rowIdx) {
        ui8* inputRow = inputPtr + rowIdx * inputMatrix->GetNumColumns();
        float* outputRow = outputPtr + rowIdx * outputMatrix->GetNumColumns();

        TBitVectorReader bitVector(inputRow);
        for (size_t colIdx = 0; colIdx < outputMatrix->GetNumColumns(); ++colIdx) {
            ui8 bin = bitVector.Get(bitsPerInput);
            outputRow[colIdx] = Min + coeff * bin;
        }
        Y_ASSERT(bitVector.GetData() == (inputRow + inputMatrix->GetNumColumns() - 1));
    }
}

void TDecompressorLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}


// ---------------------one hot compressor---------------------------
TOneHotCompressorLayer::TOneHotCompressorLayer()
    : NumBins(0)
{
}

TOneHotCompressorLayer::TOneHotCompressorLayer(const TString& input, const TString& output, size_t numBins)
    : Input(input)
    , Output(output)
    , NumBins(numBins)
{
    if (!IsPowerOfTwo(NumBins) || NumBins > 256) {
        throw yexception() << "Only 2^N NumBins <= 256 are supported by TOneHotCompressorLayer" << Endl;
    }
}

void TOneHotCompressorLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["NumBins"] = SaveToStroka(NumBins);
    ::Save(s, fields);
}

void TOneHotCompressorLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka(fields.at("NumBins"), &NumBins);
}

void TOneHotCompressorLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

namespace {
    size_t GetBinOneHot(const float* v, size_t numBins) {
        if (numBins == 0) {
            return 0;
        }

        float sum = 0;
        size_t maxBin = 0;
        for (size_t bin: xrange(numBins)) {
            float val = *(v + bin);
            if (val < 0 || val > 1) {
                throw yexception() << "Input is not in one hot representation" << Endl;
            }
            sum += val;
            if (val == 1) {
                maxBin = bin;
            }
        }
        if (sum != 1 || v[maxBin] != 1) {
            throw yexception() << "Input is not in one hot representation" << Endl;
        }
        return maxBin;
    }
}

void TOneHotCompressorLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TCharMatrix();
    }
    TCharMatrix* outputMatrix = CheckInputAndCast<TCharMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);
    if (inputMatrix->GetNumColumns() % NumBins != 0) {
        throw yexception() << "Input matrix " << inputMatrix->GetNumColumns()
            << " is not divisible by " << NumBins;
    }
    size_t bitsPerInput = GetMaxBit(NumBins - 1);
    size_t outputNumBits = bitsPerInput * inputMatrix->GetNumColumns() / NumBins;
    size_t outputNumChars = outputNumBits / 8 + (outputNumBits % 8 > 0);

    outputMatrix->Resize(inputMatrix->GetNumRows(), outputNumChars);
    outputMatrix->SetNumUnpackedColumns(inputMatrix->GetNumColumns());
    memset(outputMatrix->GetData(), 0, outputMatrix->GetSize());
    if (outputNumChars == 0) {
        return;
    }

    const float* inputPtr = inputMatrix->GetData();
    ui8* outputPtr = outputMatrix->GetData();
    for (size_t rowIdx = 0; rowIdx < inputMatrix->GetNumRows(); ++rowIdx) {
        const float* inputRow = inputPtr + rowIdx * inputMatrix->GetNumColumns();
        ui8* outputRow = outputPtr + rowIdx * outputNumChars;
        TBitVectorWriter bits(outputRow);
        for (size_t colIdx = 0; colIdx < inputMatrix->GetNumColumns(); colIdx += NumBins) {
            ui8 bin = (ui8)GetBinOneHot(inputRow + colIdx, NumBins);
            bits.Write(bin, bitsPerInput);
        }
        Y_ASSERT(bits.GetData() == (outputRow + outputNumChars - 1));
    }
}

void TOneHotCompressorLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}


// ---------------------one hot decompressor---------------------------
TOneHotDecompressorLayer::TOneHotDecompressorLayer()
    : NumBins(0)
{}

TOneHotDecompressorLayer::TOneHotDecompressorLayer(const TString& input, const TString& output, size_t numBins)
    : Input(input)
    , Output(output)
    , NumBins(numBins)
{
    if (!IsPowerOfTwo(NumBins) || NumBins > 256) {
        throw yexception() << "Only 2^N NumBins <= 256 are supported by TOneHotDecompressorLayer" << Endl;
    }
}

void TOneHotDecompressorLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["NumBins"] = SaveToStroka(NumBins);
    ::Save(s, fields);
}

void TOneHotDecompressorLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka(fields.at("NumBins"), &NumBins);
}

void TOneHotDecompressorLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TOneHotDecompressorLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TCharMatrix* inputMatrix = CheckInputAndCast<TCharMatrix*>(context, Input);
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumUnpackedColumns());
    memset(outputMatrix->GetData(), 0, sizeof(float) * outputMatrix->GetSize());
    ui8* inputPtr = inputMatrix->GetData();
    float* outputPtr = outputMatrix->GetData();
    if (outputMatrix->GetSize() == 0) {
        return;
    }

    size_t bitsPerInput = GetMaxBit(NumBins - 1);
    for (size_t rowIdx = 0; rowIdx < inputMatrix->GetNumRows(); ++rowIdx) {
        ui8* inputRow = inputPtr + rowIdx * inputMatrix->GetNumColumns();
        float* outputRow = outputPtr + rowIdx * outputMatrix->GetNumColumns();
        TBitVectorReader bitVector(inputRow);
        for (size_t colIdx = 0; colIdx < outputMatrix->GetNumColumns(); colIdx += NumBins) {
            ui8 bin = bitVector.Get(bitsPerInput);
            outputRow[colIdx + bin] = 1;
        }
        Y_ASSERT(bitVector.GetData() == (inputRow + inputMatrix->GetNumColumns() - 1));
    }
}

void TOneHotDecompressorLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

// ---------------------softmax quantize---------------------------
TSoftmaxQuantizeLayer::TSoftmaxQuantizeLayer()
    : NumBins(0)
{
}

TSoftmaxQuantizeLayer::TSoftmaxQuantizeLayer(const TString& input, const TString& output, size_t numBins)
    : Input(input)
    , Output(output)
    , NumBins(numBins)
{}

void TSoftmaxQuantizeLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["NumBins"] = SaveToStroka(NumBins);
    ::Save(s, fields);
}

void TSoftmaxQuantizeLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    LoadFromStroka(fields.at("NumBins"), &NumBins);
}

void TSoftmaxQuantizeLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TSoftmaxQuantizeLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());
    memset(outputMatrix->GetData(), 0, sizeof(float) * outputMatrix->GetSize());

    const float* inputPtr = inputMatrix->GetData();
    float* outputPtr = outputMatrix->GetData();
    for (size_t i = 0; i < inputMatrix->GetSize(); i += NumBins) {
        Y_UNUSED(i);
        size_t maxBin = 0;
        float maxVal = *inputPtr;
        for (size_t j = 1; j < NumBins; ++j) {
            if (maxVal < inputPtr[j]) {
                maxVal = inputPtr[j];
                maxBin = j;
            }
        }
        outputPtr[maxBin] = 1;
        inputPtr += NumBins;
        outputPtr += NumBins;
    }
}

void TSoftmaxQuantizeLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

// ---------------------Hadamar-like product---------------------------
THadamarlikeProductLayer::THadamarlikeProductLayer()
{
}

THadamarlikeProductLayer::THadamarlikeProductLayer(const TString& inputMatrix, const TString& inputVector, const TString& output)
    : InputMatrix(inputMatrix)
    , InputVector(inputVector)
    , Output(output)
{
}

void THadamarlikeProductLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["InputMatrix"] = InputMatrix;
    fields["InputVector"] = InputVector;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void THadamarlikeProductLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    InputMatrix = fields.at("InputMatrix");
    InputVector = fields.at("InputVector");
    Output = fields.at("Output");
}

void THadamarlikeProductLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void THadamarlikeProductLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, InputMatrix);
    const TMatrix* inputVector = CheckInputAndCast<TMatrix*>(context, InputVector);

    CheckSameRows(inputMatrix, inputVector, InputMatrix, InputVector);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        float* outputRow = (*outputMatrix)[row];
        const float* inputRow = (*inputMatrix)[row];
        float multiplier = (*inputVector)[row][0];
        for (size_t column = 0; column < inputMatrix->GetNumColumns(); ++column) {
            outputRow[column] = inputRow[column] * multiplier;
        }
    }
}

void THadamarlikeProductLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, InputMatrix);
    TryRename(name, newName, InputVector);
    TryRename(name, newName, Output);
}

// ---------------------Normalize backprop---------------------------
TNormalizeRowsGradientLayer::TNormalizeRowsGradientLayer()
{
}

TNormalizeRowsGradientLayer::TNormalizeRowsGradientLayer(const TString& input, const TString& output, const TString& outputGradient, const TString& inputGradient, double normBase)
    : Input(input)
    , Output(output)
    , OutputGradient(outputGradient)
    , InputGradient(inputGradient)
    , NormBase(normBase)
{}

void TNormalizeRowsGradientLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["OutputGradient"] = OutputGradient;
    fields["InputGradient"] = InputGradient;
    fields["NormBase"] = SaveToStroka(NormBase);
    ::Save(s, fields);
}

void TNormalizeRowsGradientLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    OutputGradient = fields.at("OutputGradient");
    InputGradient = fields.at("InputGradient");
    LoadFromStroka(fields["NormBase"], &NormBase);
}

void TNormalizeRowsGradientLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TNormalizeRowsGradientLayer::Apply(TEvalContext& context) const {
    if (!context.has(InputGradient)) {
        context[InputGradient] = new TMatrix();
    }
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    TMatrix* outputGradientMatrix = CheckInputAndCast<TMatrix*>(context, OutputGradient);
    TMatrix* inputGradientMatrix = CheckInputAndCast<TMatrix*>(context, InputGradient);

    inputGradientMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    CheckSameRows(inputMatrix, outputMatrix, Input, Output);
    CheckSameRows(inputMatrix, outputGradientMatrix, Input, OutputGradient);

    size_t numColumns = inputMatrix->GetNumColumns();

    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        float norm2 = NormBase;
        const float* inputRow = (*inputMatrix)[row];
        float* gradRow = (*outputGradientMatrix)[row];
        float* inputGradientRow = (*inputGradientMatrix)[row];
        for (size_t column = 0; column < inputMatrix->GetNumColumns(); ++column) {
            norm2 += (*inputMatrix)[row][column] * (*inputMatrix)[row][column];
        }
        norm2 = sqrt(norm2);
        { //TUnitNorm::Bprop (https://paste.yandex-team.ru/194401, ask @insight for more information)
            float norm = 1.0 / norm2;
            float norm3 = norm * norm * norm;
            float mixedSum = 0;
            const float mean = 0.0;
            const float scaler = 1.0;
            for (size_t i = 0; i < numColumns; ++i) {
                mixedSum += (inputRow[i] - mean) * gradRow[i];
            }
            for (size_t i = 0; i < numColumns; ++i) {
                float val = scaler * norm * gradRow[i];
                inputGradientRow[i] = val;
            }
            for (size_t i = 0; i < numColumns; ++i) {
                float val = scaler * (inputRow[i] - mean) * mixedSum * norm3;
                inputGradientRow[i] -= val;
            }
        }
    }
}

void TNormalizeRowsGradientLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
    TryRename(name, newName, OutputGradient);
    TryRename(name, newName, InputGradient);
}

// ---------------------Normalize backprop---------------------------
TNorm2GradientLayer::TNorm2GradientLayer(const TString& input, const TString& outputGradient, const TString& inputGradient, bool doBug)
    : Input(input)
    , OutputGradient(outputGradient)
    , InputGradient(inputGradient)
    , DoBug(doBug)
{}

void TNorm2GradientLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["OutputGradient"] = OutputGradient;
    fields["InputGradient"] = InputGradient;
    fields["DoBug"] = SaveToStroka(DoBug);
    ::Save(s, fields);
}

void TNorm2GradientLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    OutputGradient = fields.at("OutputGradient");
    InputGradient = fields.at("InputGradient");
    LoadFromStroka(fields.at("DoBug"), &DoBug);
}

void TNorm2GradientLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TNorm2GradientLayer::Apply(TEvalContext& context) const {
    if (!context.has(InputGradient)) {
        context[InputGradient] = new TMatrix();
    }
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, OutputGradient);
    TMatrix* inputGradientMatrix = CheckInputAndCast<TMatrix*>(context, InputGradient);

    inputGradientMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());
    inputGradientMatrix->FillZero();

    size_t numColumns = inputMatrix->GetNumColumns();

    if (numColumns < 2) {
        ythrow yexception() << "Norm2GradientLayer input has less then 2 columns";
    }

    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        double mean = 0.0;
        double norm2 = 1.0;
        const float* inputRow = (*inputMatrix)[row];
        float* outputRow = (*outputMatrix)[row];
        float* inputGradientRow = (*inputGradientMatrix)[row];
        mean = Accumulate(inputRow, inputRow + numColumns, 0.0) * 1.0 / numColumns;
        for (size_t column = 0; column < numColumns; ++column) {
            double value = inputRow[column];
            norm2 += (value - mean) * (value - mean);
        }
        norm2 = sqrt(norm2);

        float scaler = DoBug ? numColumns : sqrt(numColumns);

        float sumGradient = 0;
        { //TUnitNorm::Bprop (https://paste.yandex-team.ru/194401, ask @insight for more information)
            const size_t numInactiveOutputs = 2;
            float norm = 1.0 / norm2;
            float norm3 = norm * norm * norm;
            float mixedSum = 0;
            for (size_t i = 0; i < numColumns - numInactiveOutputs; ++i) {
                mixedSum += (inputRow[i] - mean) * outputRow[i];
            }
            for (size_t i = 0; i < numColumns - numInactiveOutputs; ++i) {
                float val = scaler * norm * outputRow[i];
                inputGradientRow[i] = val;
                sumGradient += val;
            }
            for (size_t i = 0; i < numColumns; ++i) {
                float val = scaler * (inputRow[i] - mean) * mixedSum * norm3;
                inputGradientRow[i] -= val;
                sumGradient -= val;
            }
        }

        float invScaler = 1.0 / numColumns;

        for (size_t i = 0; i < numColumns; ++i) {
            inputGradientRow[i] += invScaler * (-sumGradient + outputRow[numColumns - 2]);
        }

        float invSqrNorm = 1.0 / (norm2 * norm2);
        for (size_t i = 0; i < numColumns; ++i) {
            inputGradientRow[i] += outputRow[numColumns - 1] *
                (inputRow[i] - mean) * invSqrNorm;
        }
    }
}

void TNorm2GradientLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, OutputGradient);
    TryRename(name, newName, InputGradient);
}


// ---------------------Affine backprop---------------------------

TLinearTransposedLayer::TLinearTransposedLayer(const TString& input, const TString& transformMatrixInput, const TString& output)
    : Input(input)
    , TransfromMatrixInput(transformMatrixInput)
    , Output(output)
{
}
void TLinearTransposedLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["TransfromMatrixInput"] = TransfromMatrixInput;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TLinearTransposedLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    TransfromMatrixInput = fields.at("TransfromMatrixInput");
    Output = fields.at("Output");
}

void TLinearTransposedLayer::Init(const TContext& context) {
    TransfromMatrix = CheckInputAndCast<TMatrix*>(context, TransfromMatrixInput);
}

//for some reason, if we implement it directly in Apply, clang fails to vectorize it
void TransposeTransform(const TMatrix& input, TMatrix& transform, TMatrix& output) {
    output.Resize(input.GetNumRows(), transform.GetNumColumns());
    output.FillZero();

    for (size_t i = 0; i < output.GetNumRows(); ++i) {
        for (size_t k = 0; k < input.GetNumColumns(); ++k) {
            float mul = input[i][k];
            for (size_t j = 0; j < output.GetNumColumns(); ++j) {
                output[i][j] += mul * transform[k][j];
            }
        }
    }
}

void TLinearTransposedLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    Y_ASSERT(inputMatrix->GetNumColumns() == TransfromMatrix->GetNumRows());

    return TransposeTransform(*inputMatrix, *TransfromMatrix, *outputMatrix);
}

void TLinearTransposedLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, TransfromMatrixInput);
    TryRename(name, newName, Output);
}

// ---------------------Split (revert TConcat)---------------------------
TSplitLayer::TSplitLayer(const TString& input, const TVector<TString>& templates, const TVector<TString>& output)
    : Input(input)
    , Templates(templates)
    , Outputs(output)
{
}

void TSplitLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Templates"] = SaveToStroka(Templates);
    fields["Outputs"] = SaveToStroka(Outputs);
    ::Save(s, fields);
}

void TSplitLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    LoadFromStroka<TVector<TString>>(fields.at("Templates"), &Templates);
    LoadFromStroka<TVector<TString>>(fields.at("Outputs"), &Outputs);
}

void TSplitLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TSplitLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    for (TString& field : Templates) {
        TryRename(name, newName, field);
    }
    for (TString& field : Outputs) {
        TryRename(name, newName, field);
    }
}

void TSplitLayer::Apply(TEvalContext& context) const {
    Y_ASSERT(Templates.size() == Outputs.size());

    size_t columnOffset = 0;
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    if (Templates.size() != Outputs.size()) {
        ythrow yexception() << "Different number of templates and outputs for splitting " << Input;
    }

    for (size_t i = 0; i < Templates.size(); ++i) {
        TMatrix* templateMatrix = CheckInputAndCast<TMatrix*>(context, Templates[i]);
        TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Outputs[i]);
        outputMatrix->Resize(inputMatrix->GetNumRows(), templateMatrix->GetNumColumns());

        for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
            std::copy((*inputMatrix)[row] + columnOffset, (*inputMatrix)[row] + columnOffset + templateMatrix->GetNumColumns(), (*outputMatrix)[row]);
        }
        columnOffset += templateMatrix->GetNumColumns();
    }
}

// ---------------------Elementwise backprop---------------------------
template <class TFunc>
void TElementwiseGradientLayer<TFunc>::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["OutputGradient"] = OutputGradient;
    fields["InputGradient"] = InputGradient;
    ::Save(s, fields);
}

template <class TFunc>
void TElementwiseGradientLayer<TFunc>::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    OutputGradient = fields.at("OutputGradient");
    InputGradient = fields.at("InputGradient");
}

template <class TFunc>
void TElementwiseGradientLayer<TFunc>::Init(const TContext& context) {
    Y_UNUSED(context);
}

template <class TFunc>
void TElementwiseGradientLayer<TFunc>::Apply(TEvalContext& context) const {
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);
    TMatrix* outputGradientMatrix = CheckInputAndCast<TMatrix*>(context, OutputGradient);
    TMatrix* inputGradientMatrix = context.CreateOrGet<TMatrix>(InputGradient);

    CheckSameSize(inputMatrix, outputGradientMatrix, Input, OutputGradient);

    inputGradientMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t i = 0; i < inputMatrix->GetNumColumns(); ++i) {
            (*inputGradientMatrix)[row][i] = TFunc::Bprop((*inputMatrix)[row][i]) * (*outputGradientMatrix)[row][i];
        }
    }
}

template <class TFunc>
void TElementwiseGradientLayer<TFunc>::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
    TryRename(name, newName, OutputGradient);
    TryRename(name, newName, InputGradient);
}

// ---------------------Propagate sparse back (for active components only)---------------------------
TSparseMatrixToEmbeddingGradientLayer::TSparseMatrixToEmbeddingGradientLayer(
    const TString& outputGradient,
    const TString& sparseMatrixInput,
    const TString& embeddingMatrixInput,
    const TString& sparseMatrixInputGradient)
    : OutputGradient(outputGradient)
    , SparseMatrixInput(sparseMatrixInput)
    , EmbeddingMatrixInput(embeddingMatrixInput)
    , SparseMatrixInputGradient(sparseMatrixInputGradient)
{
}

void TSparseMatrixToEmbeddingGradientLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["OutputGradient"] = OutputGradient;
    fields["SparseMatrixInput"] = SparseMatrixInput;
    fields["EmbeddingMatrixInput"] = EmbeddingMatrixInput;
    fields["SparseMatrixInputGradient"] = SparseMatrixInputGradient;
    ::Save(s, fields);
}

void TSparseMatrixToEmbeddingGradientLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    OutputGradient = fields.at("OutputGradient");
    SparseMatrixInput = fields.at("SparseMatrixInput");
    EmbeddingMatrixInput = fields.at("EmbeddingMatrixInput");
    SparseMatrixInputGradient = fields.at("SparseMatrixInputGradient");
}

void TSparseMatrixToEmbeddingGradientLayer::Init(const TContext& context) {
    TString embeddingTypeName = context.at(EmbeddingMatrixInput)->GetTypeName();
    if (embeddingTypeName == "TMatrix") {
        EmbeddingMatrix = CheckInputAndCast<TMatrix*>(context, EmbeddingMatrixInput);
        CharEmbeddingMatrix = nullptr;
    } else if (embeddingTypeName == "TCharMatrix") {
        EmbeddingMatrix = nullptr;
        CharEmbeddingMatrix = CheckInputAndCast<TCharMatrix*>(context, EmbeddingMatrixInput);
    } else {
        ythrow yexception() << "Bad embedding matrix type: " << embeddingTypeName;
    }
}

void TSparseMatrixToEmbeddingGradientLayer::Apply(TEvalContext& context) const {
    if (!context.has(SparseMatrixInputGradient)) {
        context[SparseMatrixInputGradient] = new TSparseMatrix();
    }
    TSparseMatrix* inputGradientMatrix = CheckInputAndCast<TSparseMatrix*>(context, SparseMatrixInputGradient);

    TSparseMatrix* inputMatrix = CheckInputAndCast<TSparseMatrix*>(context, SparseMatrixInput);
    TMatrix* outputGradientMatrix = CheckInputAndCast<TMatrix*>(context, OutputGradient);
    const TVector<TSparseVector>& vectors = inputMatrix->Vectors;

    size_t embeddingColumns = EmbeddingMatrix != nullptr
        ? EmbeddingMatrix->GetNumColumns()
        : CharEmbeddingMatrix->GetNumColumns();
    size_t embeddingRows = EmbeddingMatrix != nullptr
        ? EmbeddingMatrix->GetNumRows()
        : CharEmbeddingMatrix->GetNumRows();

    inputGradientMatrix->Vectors.resize(vectors.size());
    TVector<float> embeddingRowVector;
    for (size_t inputId = 0; inputId < vectors.size(); ++inputId) {
        Y_ASSERT(vectors[inputId].Indexes.size() == vectors[inputId].Values.size());
        auto& indexes = inputGradientMatrix->Vectors[inputId].Indexes;
        indexes.resize(vectors[inputId].Indexes.size());
        auto& values = inputGradientMatrix->Vectors[inputId].Values;
        values.resize(vectors[inputId].Indexes.size());
        for (size_t i = 0; i < vectors[inputId].Indexes.size(); ++i) {
            size_t index =  vectors[inputId].Indexes[i];
            if (index >= embeddingRows) {
                continue;
            }
            float* embeddingRow = nullptr;
            if (EmbeddingMatrix != nullptr) {
               embeddingRow = (*EmbeddingMatrix)[index];
            } else {
                CharEmbeddingMatrix->GetRow(index, embeddingRowVector);
                embeddingRow = &embeddingRowVector[0];
            }
            indexes[i] = index;
            values[i] = DotProduct(embeddingRow, (*outputGradientMatrix)[inputId], embeddingColumns);
        }
    }
}

void TSparseMatrixToEmbeddingGradientLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, OutputGradient);
    TryRename(name, newName, SparseMatrixInput);
    TryRename(name, newName, EmbeddingMatrixInput);
    TryRename(name, newName, SparseMatrixInputGradient);
}

// Alternative embedder, which saves token positions
// may be integrate it to TStringToSparseMatrix?..
TStringToSparseMatrixAndPosLayer::TStringToSparseMatrixAndPosLayer(const TString& textsInput,
    const TString& sparsifierInput,
    const TString& output,
    const TString& outputPositions)
    : TextsInput(textsInput)
    , SparsifierInput(sparsifierInput)
    , Output(output)
    , OutputPositions(outputPositions)
{
}

void TStringToSparseMatrixAndPosLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["TextsInput"] = TextsInput;
    fields["SparsifierInput"] = SparsifierInput;
    fields["Output"] = Output;
    fields["OutputPositions"] = OutputPositions;
    ::Save(s, fields);
}

void TStringToSparseMatrixAndPosLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    TextsInput = fields.at("TextsInput");
    SparsifierInput = fields.at("SparsifierInput");
    Output = fields.at("Output");
    OutputPositions = fields.at("OutputPositions");
}

void TStringToSparseMatrixAndPosLayer::Init(const TContext& context) {
    Sparsifier = CheckInputAndCast<TSparsifier*>(context, SparsifierInput);
}

void TStringToSparseMatrixAndPosLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TSparseMatrix();
    }
    if (!context.has(OutputPositions)) {
        context[OutputPositions] = new TPositionedOneHotEncodingMatrix();
    }
    TTextsVector* textsVector = CheckInputAndCast<TTextsVector*>(context, TextsInput);
    TSparseMatrix* outputSparseMatrix = CheckInputAndCast<TSparseMatrix*>(context, Output);
    TPositionedOneHotEncodingMatrix* outputPositionsMatrix = CheckInputAndCast<TPositionedOneHotEncodingMatrix*>(context, OutputPositions);

    const TVector<TString>& texts = textsVector->Texts;

    outputSparseMatrix->Vectors.resize(texts.size());
    outputPositionsMatrix->Encodings.resize(texts.size());

    for (size_t textId = 0; textId < texts.size(); ++textId) {
        outputSparseMatrix->Vectors[textId].Indexes.clear();
        outputSparseMatrix->Vectors[textId].Values.clear();
        outputPositionsMatrix->Encodings[textId].clear();

        const TString& curText = texts[textId];
        Sparsifier->ToSparse(curText, outputSparseMatrix->Vectors[textId], &outputPositionsMatrix->Encodings[textId]);
        Sort(outputPositionsMatrix->Encodings[textId].begin(), outputPositionsMatrix->Encodings[textId].end(),
                [](const TPositionedOneHotEncoding& a, const TPositionedOneHotEncoding& b) {
                    //from left to right, from big to small, from phrases to char trigrams
                    return std::tie(a.Region.Begin, b.Region.End, a.Type) < std::tie(b.Region.Begin, a.Region.End, b.Type);
                }
        );
    }
}

void TStringToSparseMatrixAndPosLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, TextsInput);
    TryRename(name, newName, SparsifierInput);
    TryRename(name, newName, Output);
    TryRename(name, newName, OutputPositions);
}

void TWeightsJoinOptions::Save(IOutputStream* s) const {
    ::Save(s, WordWeight);
    ::Save(s, Trigrams3Weight);
    ::Save(s, Trigrams2Weight);
    ::Save(s, Trigrams1Weight);
    ::Save(s, BigramsWeight);
}

void TWeightsJoinOptions::Load(IInputStream* s) {
    ::Load(s, WordWeight);
    ::Load(s, Trigrams3Weight);
    ::Load(s, Trigrams2Weight);
    ::Load(s, Trigrams1Weight);
    ::Load(s, BigramsWeight);
}

//Join coordinates corresponding to one-hot encodings of different types
TJoinMultiplySparsesAndPositionsLayer::TJoinMultiplySparsesAndPositionsLayer(
        const TVector<TString>& inputsSparse,
        const TVector<TString>& inputsPosition,
        const TString& outputSparse,
        const TString& outputPositions
)
    : InputsSparse(inputsSparse)
    , InputsPosition(inputsPosition)
    , OutputSparse(outputSparse)
    , OutputPositions(outputPositions)
{
    if (InputsSparse.size() != InputsPosition.size()) {
        ythrow yexception() << "Different number of sparse and position inputs: " << InputsSparse.size() << " and " << InputsPosition.size();
    }
}

void TJoinMultiplySparsesAndPositionsLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["InputsSparse"] = SaveToStroka(InputsSparse);
    fields["InputsPosition"] = SaveToStroka(InputsPosition);
    fields["OutputSparse"] = OutputSparse;
    fields["OutputPositions"] = OutputPositions;
    ::Save(s, fields);
}

void TJoinMultiplySparsesAndPositionsLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    LoadFromStroka(fields["InputsSparse"], &InputsSparse);
    LoadFromStroka(fields["InputsPosition"], &InputsPosition);
    if (InputsSparse.size() != InputsPosition.size()) {
        ythrow yexception() << "Different number of sparse and position inputs: " << InputsSparse.size() << " and " << InputsPosition.size();
    }
    OutputSparse = fields.at("OutputSparse");
    OutputPositions = fields.at("OutputPositions");
}

void TJoinMultiplySparsesAndPositionsLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TJoinMultiplySparsesAndPositionsLayer::Apply(TEvalContext& context) const {
    if (!context.has(OutputSparse)) {
        context[OutputSparse] = new TSparseMatrix();
    }
    TSparseMatrix* outputSparseMatrix = CheckInputAndCast<TSparseMatrix*>(context, OutputSparse);
    if (!context.has(OutputPositions)) {
        context[OutputPositions] = new TPositionedOneHotEncodingMatrix();
    }
    TPositionedOneHotEncodingMatrix* outputPositionsMatrix = CheckInputAndCast<TPositionedOneHotEncodingMatrix*>(context, OutputPositions);
    outputSparseMatrix->Vectors.clear();
    outputPositionsMatrix->Encodings.clear();

    TVector<TSparseMatrix*> inputSparseMatrices;
    for (auto& input : InputsSparse) {
        inputSparseMatrices.push_back(CheckInputAndCast<TSparseMatrix*>(context, input));
    }
    TVector<TPositionedOneHotEncodingMatrix*> inputPositionMatrices;
    for (auto& input : InputsPosition) {
        inputPositionMatrices.push_back(CheckInputAndCast<TPositionedOneHotEncodingMatrix*>(context, input));
    }

    if (InputsSparse.empty()) {
        return;
    }
    size_t rows = inputSparseMatrices[0]->Vectors.size();
    for (const auto sparseMatrix : inputSparseMatrices) {
        if (sparseMatrix->Vectors.size() != rows) {
            ythrow yexception() << "Different number of rows in different inputs matrices: " << rows << " and " << sparseMatrix->Vectors.size();
        }
    }
    for (const auto positionsMatrix : inputPositionMatrices) {
        if (positionsMatrix->Encodings.size() != rows) {
            ythrow yexception() << "Different number of rows in different inputs matrices: " << rows << " and " << positionsMatrix->Encodings.size();
        }
    }
    outputSparseMatrix->Vectors.resize(rows);
    outputPositionsMatrix->Encodings.resize(rows);

    for (size_t row = 0; row < rows; ++row) {
        size_t prevIndexes = 0;
        for (size_t input = 0; input < InputsSparse.size(); ++input) {
            THashMap<size_t, size_t> indexesMap(inputSparseMatrices.size());
            const auto& sparseVector = inputSparseMatrices[input]->Vectors[row];
            for (size_t i = 0; i < sparseVector.Indexes.size(); ++i) {
                if (!indexesMap.contains(sparseVector.Indexes[i])) {
                    indexesMap[sparseVector.Indexes[i]] = prevIndexes + indexesMap.size();
                }
                outputSparseMatrix->Vectors[row].Indexes.push_back(indexesMap[sparseVector.Indexes[i]]);
                outputSparseMatrix->Vectors[row].Values.push_back(sparseVector.Values[i]);
            }
            const auto& positionsVector = inputPositionMatrices[input]->Encodings[row];
            for (auto position : positionsVector) {
                if (!indexesMap.contains(position.Index)) {
                    indexesMap[position.Index] = prevIndexes + indexesMap.size();
                }
                position.Index = indexesMap[position.Index];
                outputPositionsMatrix->Encodings[row].push_back(position);
            }
            prevIndexes += indexesMap.size();
        }
    }
}

void TJoinMultiplySparsesAndPositionsLayer::RenameVariable(const TString& name, const TString& newName) {
    for (auto& s : InputsSparse) {
        TryRename(name, newName, s);
    }
    for (auto& s : InputsPosition) {
        TryRename(name, newName, s);
    }
    TryRename(name, newName, OutputSparse);
    TryRename(name, newName, OutputPositions);
}

//Collects target derivatives from words, trigrams, etc. to single (sparse) vector
TJoinSparseGradsLayer::TJoinSparseGradsLayer(const TString& input, const TString& positions, const TString& output, const TWeightsJoinOptions& options)
    : Input(input)
    , Positions(positions)
    , Output(output)
    , JoinOptions(options)
{}

void TJoinSparseGradsLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Positions"] = Positions;
    fields["Output"] = Output;
    fields["JoinOptions"] = SaveToStroka(JoinOptions);
    ::Save(s, fields);
}

void TJoinSparseGradsLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Positions = fields.at("Positions");
    Output = fields.at("Output");
    LoadFromStroka(fields.at("JoinOptions"), &JoinOptions);
}

void TJoinSparseGradsLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TJoinSparseGradsLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TSparseMatrix();
    }
    TSparseMatrix* outputMatrix = CheckInputAndCast<TSparseMatrix*>(context, Output);

    TSparseMatrix* input = CheckInputAndCast<TSparseMatrix*>(context, Input);
    TPositionedOneHotEncodingMatrix* positionsMatrix = CheckInputAndCast<TPositionedOneHotEncodingMatrix*>(context, Positions);

    Y_ASSERT(input->Vectors.size() == positionsMatrix->Encodings.size());

    outputMatrix->Vectors.resize(input->Vectors.size());

    for (size_t i = 0; i < input->Vectors.size(); ++i) {
        const auto& row = input->Vectors[i];
        auto& outRow = outputMatrix->Vectors[i];
        outRow.Indexes.clear();
        outRow.Values.clear();

        auto getValue = [&row](size_t index) {
            auto pos = std::find(row.Indexes.begin(), row.Indexes.end(), index);
            if (pos == row.Indexes.end()) {
                ythrow yexception() << "no such index: " << index;
            }
            return row.Values[pos - row.Indexes.begin()];
        };

        //TODO(mihaild): speed up if necessary
        TVector<TSizeTRegion> wordsRegions;
        for (const auto& pos : positionsMatrix->Encodings[i]) {
            if (pos.Type == ETokenType::Word) {
                wordsRegions.push_back(pos.Region);
                outRow.Indexes.push_back(pos.Index);
                outRow.Values.push_back(getValue(pos.Index) * JoinOptions.WordWeight);
            }
        }
        for (const auto& pos : positionsMatrix->Encodings[i]) {
            if (pos.Type == ETokenType::Trigram) {
                size_t pb = pos.Region.Begin, pe = pos.Region.End;
                for (size_t j = 0; j < wordsRegions.size(); ++j) {
                    size_t wb = wordsRegions[j].Begin, we = wordsRegions[j].End;
                    if (pb >= wb && pe <= we) { //inside
                        outRow.Values[j] += JoinOptions.Trigrams3Weight * getValue(pos.Index);
                    } else if (pb + 1 == wb && pe <= we || pb >= wb && we + 1 == pe) {//off-by-one
                        outRow.Values[j] += JoinOptions.Trigrams2Weight * getValue(pos.Index);
                    } else if (pb + 2 == wb || we + 2 == pe || (pb + 1 == wb && we + 1 == pe)) {//off-by-two
                        outRow.Values[j] += JoinOptions.Trigrams1Weight * getValue(pos.Index);
                    }
                }
            } else if (pos.Type == ETokenType::Bigram) {
                size_t pb = pos.Region.Begin, pe = pos.Region.End;
                for (size_t j = 0; j < wordsRegions.size(); ++j) {
                    size_t wb = wordsRegions[j].Begin, we = wordsRegions[j].End;
                    if (pb <= wb && pe >= we) {
                        outRow.Values[j] += JoinOptions.BigramsWeight * getValue(pos.Index);
                    }
                }
            }
        }
    }
}

void TJoinSparseGradsLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Positions);
    TryRename(name, newName, Output);
};

// Collects a statistic from sparse layer
template<class TAggregateFunc>
void TSparseMatrixStatisticLayer<TAggregateFunc>::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    ::Save(s, fields);
}

template<class TAggregateFunc>
void TSparseMatrixStatisticLayer<TAggregateFunc>::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
}

template<class TAggregateFunc>
void TSparseMatrixStatisticLayer<TAggregateFunc>::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);

    TSparseMatrix* input = CheckInputAndCast<TSparseMatrix*>(context, Input);

    outputMatrix->Resize(input->Vectors.size(), TAggregateFunc::OUTPUTS);

    for (size_t i = 0; i < input->Vectors.size(); ++i) {
        const auto& statistics = TAggregateFunc::Func(input->Vectors[i].Values);
        std::copy(statistics.begin(), statistics.end(), (*outputMatrix)[i]);
    }
}

template<class TAggregateFunc>
void TSparseMatrixStatisticLayer<TAggregateFunc>::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
};

// --------------------------- round to precision layer -----------------------------
TRoundToPrecisionLayer::TRoundToPrecisionLayer(const TString& input, const TString& precision, const TString& output)
    : Input{input}
    , Precision{precision}
    , Output{output}
{}

void TRoundToPrecisionLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Precision"] = Precision;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TRoundToPrecisionLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Precision = fields["Precision"];
    Output = fields.at("Output");
}

void TRoundToPrecisionLayer::Init(const TContext& context) {
    if (!context.has(Precision)) {
        return;
    }
    TMatrix* matrix = CheckInputAndCast<TMatrix*>(context, Precision);
    PrecisionValue = (*matrix)[0];
    MultiplierValue = std::pow(10.0f, *PrecisionValue);
}

void TRoundToPrecisionLayer::Apply(TEvalContext& context) const {
    if (!PrecisionValue) {
        context[Output] = context.at(Input);
        return;
    }

    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);
    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    // TODO(olegator): Speed this up if needed
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t column = 0; column < inputMatrix->GetNumColumns(); ++column) {
            (*outputMatrix)[row][column] = RoundToPrecision((*inputMatrix)[row][column]);
        }
    }
}

void TRoundToPrecisionLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Precision);
    TryRename(name, newName, Output);
}

float TRoundToPrecisionLayer::RoundToPrecision(float value) const noexcept {
    if (value < 0.0f) {
        return static_cast<i64>(value * MultiplierValue - 0.5f) / MultiplierValue;
    }
    return static_cast<i64>(value * MultiplierValue + 0.5f) / MultiplierValue;
}

// --------------------------- batch normalization layer -----------------------------
TBatchNormLayer::TBatchNormLayer(
    const TString& input,
    const TString& meanVectorInput,
    const TString& varVectorInput,
    const TString& betaVectorInput,
    const TString& gammaVectorInput,
    const TString& output)
    : Input(input)
    , MeanVectorInput(meanVectorInput)
    , VarVectorInput(varVectorInput)
    , BetaVectorInput(betaVectorInput)
    , GammaVectorInput(gammaVectorInput)
    , Output(output)
{
}

void TBatchNormLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["MeanVectorInput"] = MeanVectorInput;
    fields["VarVectorInput"] = VarVectorInput;
    fields["BetaVectorInput"] = BetaVectorInput;
    fields["GammaVectorInput"] = GammaVectorInput;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TBatchNormLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    MeanVectorInput = fields.at("MeanVectorInput");
    VarVectorInput = fields.at("VarVectorInput");
    BetaVectorInput = fields.at("BetaVectorInput");
    GammaVectorInput = fields.at("GammaVectorInput");
    Output = fields.at("Output");
}

void TBatchNormLayer::Init(const TContext& context) {
    Mean = CheckInputAndCast<TMatrix*>(context, MeanVectorInput);
    Var = CheckInputAndCast<TMatrix*>(context, VarVectorInput);
    Beta = CheckInputAndCast<TMatrix*>(context, BetaVectorInput);
    Gamma = CheckInputAndCast<TMatrix*>(context, GammaVectorInput);

    CheckSameSize(Mean, Var, MeanVectorInput, VarVectorInput);
    CheckSameSize(Beta, Gamma, BetaVectorInput, GammaVectorInput);
    CheckSameSize(Mean, Beta, MeanVectorInput, BetaVectorInput);
}

void TBatchNormLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    CheckSameColumns(inputMatrix, Mean, Input, MeanVectorInput);

    Y_ASSERT(Mean->GetNumRows() == 1);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    // TODO(boyalex): Speed this up
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t column = 0; column < inputMatrix->GetNumColumns(); ++column) {
            (*outputMatrix)[row][column] =
                ((*inputMatrix)[row][column] - (*Mean)[0][column]) / sqrt((*Var)[0][column] + 0.001)
                * (*Gamma)[0][column]
                + (*Beta)[0][column];
        }
    }
}

void TBatchNormLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, MeanVectorInput);
    TryRename(name, newName, VarVectorInput);
    TryRename(name, newName, BetaVectorInput);
    TryRename(name, newName, GammaVectorInput);
    TryRename(name, newName, Output);
}

// --------------------------- linear combination layer -----------------------------
TLinearCombinationLayer::TLinearCombinationLayer(
    const TVector<TString>& inputs,
    const TString& weightsMatrixInput,
    const TString& output)
    : Inputs(inputs)
    , WeightsMatrixInput(weightsMatrixInput)
    , Output(output)
{
}

void TLinearCombinationLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Inputs"] = SaveToStroka(Inputs);
    fields["WeightsMatrixInput"] = WeightsMatrixInput;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TLinearCombinationLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    LoadFromStroka(fields["Inputs"], &Inputs);
    WeightsMatrixInput = fields.at("WeightsMatrixInput");
    Output = fields.at("Output");
}

TVector<TString> TLinearCombinationLayer::GetInputs() const {
    TVector<TString> inputs = Inputs;
    inputs.push_back(WeightsMatrixInput);
    return inputs;
}

void TLinearCombinationLayer::Init(const TContext& context) {
    Weights = CheckInputAndCast<TMatrix*>(context, WeightsMatrixInput);
}

void TLinearCombinationLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix1 = CheckInputAndCast<TMatrix*>(context, Inputs[0]);

    CheckSameColumns(inputMatrix1, Weights, Inputs[0], WeightsMatrixInput);

    Y_ASSERT(Inputs.size() == Weights->GetNumRows());

    outputMatrix->Resize(inputMatrix1->GetNumRows(), inputMatrix1->GetNumColumns());
    outputMatrix->FillZero();

    // TODO(boyalex): Speed this up
    for (size_t i = 0; i < Inputs.size(); ++i) {
        const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Inputs[i]);
        CheckSameSize(inputMatrix, outputMatrix, Inputs[i], Output);
        for (size_t row = 0; row < outputMatrix->GetNumRows(); ++row) {
            for (size_t column = 0; column < outputMatrix->GetNumColumns(); ++column) {
                (*outputMatrix)[row][column] += (*inputMatrix)[row][column] * (*Weights)[i][column];
            }
        }
    }
}

void TLinearCombinationLayer::RenameVariable(const TString& name, const TString& newName) {
    for (size_t i = 0; i < Inputs.size(); ++i) {
        TryRename(name, newName, Inputs[i]);
    }
    TryRename(name, newName, WeightsMatrixInput);
    TryRename(name, newName, Output);
}

// --------------------------- matmul layer -----------------------------
TMatMulLayer::TMatMulLayer(const TString& input1, const TString& input2, const TString& output)
    : Input1(input1)
    , Input2(input2)
    , Output(output)
{
}

void TMatMulLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input1"] = Input1;
    fields["Input2"] = Input2;
    fields["Output"] = Output;
    ::Save(s, fields);
}

void TMatMulLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input1 = fields.at("Input1");
    Input2 = fields.at("Input2");
    Output = fields.at("Output");
}

void TMatMulLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TMatMulLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);

    const TMatrix* inputMatrix1 = CheckInputAndCast<TMatrix*>(context, Input1);
    const TMatrix* inputMatrix2 = CheckInputAndCast<TMatrix*>(context, Input2);

    CheckSameColumns(inputMatrix1, inputMatrix2, Input1, Input2);

    outputMatrix->Resize(inputMatrix1->GetNumRows(), inputMatrix2->GetNumRows());

    for (size_t row = 0; row < inputMatrix1->GetNumRows(); ++row) {
        for (size_t column = 0; column < inputMatrix2->GetNumRows(); ++column) {
            const float dot = DotProduct((*inputMatrix1)[row], (*inputMatrix2)[column], inputMatrix1->GetNumColumns());
            (*outputMatrix)[row][column] = dot;
        }
    }
}

void TMatMulLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input1);
    TryRename(name, newName, Input2);
    TryRename(name, newName, Output);
}

// --------------------------- parametric elu layer -----------------------------
TParamEluLayer::TParamEluLayer(const TString& input, const TString& output, double alpha)
    : Input(input)
    , Output(output)
    , Alpha(alpha)
{
}

void TParamEluLayer::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input"] = Input;
    fields["Output"] = Output;
    fields["Alpha"] = SaveToStroka(Alpha);

    ::Save(s, fields);
}

void TParamEluLayer::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input = fields.at("Input");
    Output = fields.at("Output");
    if (fields.contains("Alpha")) {
        LoadFromStroka(fields["Alpha"], &Alpha);
    } else {
        Alpha = 1.0;
    }
}

void TParamEluLayer::Init(const TContext& context) {
    Y_UNUSED(context);
}

void TParamEluLayer::Apply(TEvalContext& context) const {
    if (!context.has(Output)) {
        context[Output] = new TMatrix();
    }
    TMatrix* outputMatrix = CheckInputAndCast<TMatrix*>(context, Output);
    const TMatrix* inputMatrix = CheckInputAndCast<TMatrix*>(context, Input);

    outputMatrix->Resize(inputMatrix->GetNumRows(), inputMatrix->GetNumColumns());

    // TODO(boyalex): speed up
    for (size_t row = 0; row < inputMatrix->GetNumRows(); ++row) {
        for (size_t column = 0; column < inputMatrix->GetNumColumns(); ++column) {
            const double value = (*inputMatrix)[row][column];
            if (value >= 0) {
                (*outputMatrix)[row][column] = value;
            } else {
                (*outputMatrix)[row][column] = Alpha * (exp(value) - 1);
            }
        }
    }
}

void TParamEluLayer::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input);
    TryRename(name, newName, Output);
}

template<class TFunc>
void TElementwiseBinaryTransform<TFunc>::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["Input1"] = Input1;
    fields["Input2"] = Input2;
    fields["Output"] = Output;
    ::Save(s, fields);
}

template<class TFunc>
void TElementwiseBinaryTransform<TFunc>::Load(TBlob& blob) {
    THashMap<TString, TString> fields = ReadFields32(blob);
    Input1 = fields.at("Input1");
    Input2 = fields.at("Input2");
    Output = fields.at("Output");
}

template<class TFunc>
void TElementwiseBinaryTransform<TFunc>::Apply(NNeuralNetApplier::TEvalContext& context) const {
    TMatrix* outputMatrix = context.CreateOrGet<TMatrix>(Output);

    const TMatrix* inputMatrix1 = CheckInputAndCast<TMatrix*>(context, Input1);
    const TMatrix* inputMatrix2 = CheckInputAndCast<TMatrix*>(context, Input2);
    CheckSameSize(inputMatrix1, inputMatrix2, Input1, Input2);

    outputMatrix->Resize(inputMatrix1->GetNumRows(), inputMatrix1->GetNumColumns());
    const float* inputData1 = inputMatrix1->GetData();
    const float* inputData2 = inputMatrix2->GetData();
    float* outputData = outputMatrix->GetData();
    for (size_t i : xrange(inputMatrix1->GetFlatSize())) {
        outputData[i] = TFunc::Fprop(inputData1[i], inputData2[i]);
    }
}

template<class TFunc>
void TElementwiseBinaryTransform<TFunc>::RenameVariable(const TString& name, const TString& newName) {
    TryRename(name, newName, Input1);
    TryRename(name, newName, Input2);
    TryRename(name, newName, Output);
}

template class TElementwiseBinaryTransform<TMul>;
template class TElementwiseBinaryTransform<TAbsDiff>;

void TModel::CopyModel(const TModel& other) {
    Inputs = other.Inputs;
    Blob = other.Blob;
    Layers.clear();
    for (auto& layer : other.Layers) {
        Layers.push_back(layer->Clone());
    }
    Parameters = other.Parameters;
    for (const auto& var : Inputs) {
        if (Parameters.has(var)) {
            Parameters[var] = CreateState(other.Parameters.at(var)->GetTypeName());
        }
    }
    SetSupportedVersions(other.GetSupportedVersions());
    SetMetaData(other.GetMetaData());
    Init();
}

TModel::TModel(const TModel& other) {
    CopyModel(other);
}

void TModel::operator=(const TModel& other) {
    if (&other == this) {
        return;
    }
    CopyModel(other);
}

TString TModel::LayersString() const {
    TStringStream s;
    ::Save(&s, Layers.size());
    for (auto& layer : Layers) {
        ::Save(&s, layer->GetTypeName());
        layer->Save(&s);
    }
    return TString(s.Data(), s.Size());
}

void TModel::Init() {
    for (auto& layer : Layers) {
        layer->Init(Parameters);
    }
    InitAllVariablesCache();
}

void TModel::FillEvalContextFromSample(TAtomicSharedPtr<ISample> sample, TEvalContext& evalContext) const {
    Y_ENSURE(!Inputs.empty());

    auto& input = evalContext[Inputs[0]];

    input = new TSamplesVector();
    TSamplesVector* samplesVector = VerifyDynamicCast<TSamplesVector*>(input.Get());
    samplesVector->SetSamples({std::move(sample)});
}

void TModel::FillEvalContextFromSamples(const TVector<TAtomicSharedPtr<ISample>>& samples, TEvalContext& evalContext) const {
    Y_ENSURE(!Inputs.empty());

    auto& input = evalContext[Inputs[0]];

    input = new TSamplesVector();
    TSamplesVector* samplesVector = VerifyDynamicCast<TSamplesVector*>(input.Get());
    samplesVector->SetSamples(samples);
}

void TModel::Apply(TEvalContext& evalContext, const TVector<TString>& outputVariables,
    TVector<float>& result) const
{
    Apply(evalContext);

    result.clear();
    result.reserve(outputVariables.size());
    for (const auto& var : outputVariables) {
        TMatrix* output = VerifyDynamicCast<TMatrix*>(
            evalContext.at(var).Get());
        Y_ASSERT(output->GetNumRows() == 1);

        for (size_t column = 0; column < output->GetNumColumns(); ++column) {
            result.push_back((*output)[0][column]);
        }
    }
}

void TModel::Apply(TEvalContext& evalContext, const TVector<TString>& outputVariables,
    TVector<TVector<float>>& result) const
{
    Apply(evalContext);

    result.clear();
    for (const auto& var : outputVariables) {
        TMatrix* output = VerifyDynamicCast<TMatrix*>(evalContext.at(var).Get());
        if (result.empty()) {
            result.resize(output->GetNumRows());
        } else {
            Y_ENSURE(result.size() == output->GetNumRows());
        }
        for (size_t row = 0; row < output->GetNumRows(); ++row) {
            const size_t rowSize = result[row].size();
            result[row].resize(rowSize + output->GetNumColumns());
            for (size_t column = 0; column < output->GetNumColumns(); ++column) {
                result[row][rowSize + column] = (*output)[row][column];
            }
        }
    }
}

void TModel::Apply(TEvalContext& evalContext, const TVector<TString>& outputVariables,
    TVector<TVector<TVector<float>>>& result) const
{
    Apply(evalContext);

    result.clear();
    result.resize(outputVariables.size());
    for (const size_t variableId : xrange(outputVariables.size())) {
        TMatrix* output = VerifyDynamicCast<TMatrix*>(evalContext.at(outputVariables[variableId]).Get());
        result[variableId].resize(output->GetNumRows());
        for (size_t row = 0; row < output->GetNumRows(); ++row) {
            result[variableId][row].resize(output->GetNumColumns());
            CopyN((*output)[row], output->GetNumColumns(), result[variableId][row].begin());
        }
    }
}

void TModel::Apply(TAtomicSharedPtr<ISample> sample, const TVector<TString>& outputVariables,
    TVector<float>& result) const
{
    TEvalContext evalContext(&Parameters, AllVariablesSet.size());
    FillEvalContextFromSample(std::move(sample), evalContext);
    Apply(evalContext, outputVariables, result);
}

void TModel::Apply(const TVector<TAtomicSharedPtr<ISample>>& samples, const TVector<TString>& outputVariables,
    TVector<TVector<float>>& result) const
{
    TEvalContext evalContext(&Parameters, AllVariablesSet.size());
    FillEvalContextFromSamples(samples, evalContext);
    Apply(evalContext, outputVariables, result);
}

void TModel::Apply(const TVector<TAtomicSharedPtr<ISample>>& samples, const TVector<TString>& outputVariables,
    TVector<TVector<TVector<float>>>& result) const
{
    TEvalContext evalContext(&Parameters, AllVariablesSet.size());
    FillEvalContextFromSamples(samples, evalContext);
    Apply(evalContext, outputVariables, result);
}

void TModel::Apply(TEvalContext& evalContext) const
{
    evalContext.SetParameters(&Parameters);
    for (auto& layer : Layers) {
        layer->Apply(evalContext);
    }
}

void TModel::Apply(TAtomicSharedPtr<ISample> sample,
    const TVector<TString>& outputVariables, TVector<ui8>& result) const
{
    TEvalContext evalContext(&Parameters, AllVariablesSet.size());
    FillEvalContextFromSample(std::move(sample), evalContext);

    Apply(evalContext);

    result.clear();
    for (const auto& var : outputVariables) {
        TCharMatrix* outputVar = VerifyDynamicCast<TCharMatrix*>(evalContext.at(var).Get());

        const auto numRows = outputVar->GetNumRows();
        const auto numCols = outputVar->GetNumColumns();
        const auto preSize = result.size();
        result.resize(preSize + numCols * numRows);

        auto outputPtr = std::next(result.data(), preSize);
        for (auto row = 0u; row < numRows; ++row, outputPtr += numCols) {
            CopyN((*outputVar)[row], numCols, outputPtr);
        }
    }
}

void TModel::Apply(TAtomicSharedPtr<ISample> sample, TVector<float>& result) const {
    Apply(std::move(sample), { "joint_output" }, result);
}

void TModel::Save(IOutputStream* s, const TVector<TString>& compressMatrixes, double compressQuantile, float deletionPercent) const {
    TStringStream info;
    THashMap<TString, TString> fields;
    fields["VERSION"] = SaveToStroka(FORMAT_VERSION);
    fields["Inputs"] = SaveToStroka(Inputs);
    fields["SupportedVersions"] = SaveToStroka(SupportedVersions);
    fields["MetaData"] = SaveToStroka(MetaData);
    ::Save(&info, fields);
    info << LayersString();
    *s << info.Str();
    Parameters.Save(s, compressMatrixes, info.Size(), compressQuantile, deletionPercent);
}

TModel TModel::FromFile(const TString& filename, bool tryLockMemory) {
    TModel result;
    auto blob = TBlob::PrechargedFromFile(filename);
    if (tryLockMemory) {
        try {
            LockMemory(blob.Data(), blob.Size());
        } catch (...) {
        }
    }
    result.LoadNoLock(blob);
    return result;
}

void TModel::Load(const TBlob& blob, const TLoadParams& loadParams) {
    try {
        LockMemory(blob.Data(), blob.Size());
    } catch (...) {
    }
    LoadNoLock(blob, loadParams);
}

void TModel::LoadNoLock(const TBlob& blob, const TLoadParams& loadParams) {
    Blob = blob;
    TBlob curBlob = blob;

    THashMap<TString, TString> fields = ReadFields32(curBlob);

    size_t modelVersion = 0;
    if (fields.contains("VERSION")) {
        LoadFromStroka<size_t>(fields.at("VERSION"), &modelVersion);
    }
    if (modelVersion == 0) {
        ythrow TInvalidFormatException();
    }
    if (modelVersion > FORMAT_VERSION) {
        ythrow yexception() << "Code version = " << FORMAT_VERSION
            << " while modelVersion = " << modelVersion;
    }
    if (fields.contains("SupportedVersions")) {
        LoadFromStroka(fields.at("SupportedVersions"), &SupportedVersions);
    }
    if (fields.contains("MetaData")) {
        LoadFromStroka(fields.at("MetaData"), &MetaData);
    }

    size_t size = ReadSize(curBlob);
    Layers.resize(size);

    for (size_t i = 0; i < size; ++i) {
        TString typeName = ReadString32(curBlob);
        ILayerPtr curLayer = CreateLayer(typeName);
        curLayer->Load(curBlob);
        Layers[i] = curLayer;
    }

    if (fields.contains("Inputs")) {
        LoadFromStroka(fields.at("Inputs"), &Inputs);
    } else {
        if (Layers.size() > 0) {
            Inputs = Layers[0]->GetInputs();
        } else {
            Inputs.clear();
        }
    }

    Parameters.Load(curBlob, loadParams);

    Y_ASSERT(curBlob.Empty());

    Init();
}

TVector<TString> TModel::AllVariables() const {
    return {AllVariablesSet.cbegin(), AllVariablesSet.cend()};
}

size_t TModel::AllVariablesCount() const {
    return AllVariablesSet.size();
}

void TModel::InitAllVariablesCache() {
    AllVariablesSet.clear();
    for (auto& layer : Layers) {
        TVector<TString> inputs = layer->GetInputs();
        TVector<TString> outputs = layer->GetOutputs();
        for (auto& s : inputs) {
            AllVariablesSet.insert(s);
        }
        for (auto& s : outputs) {
            AllVariablesSet.insert(s);
        }
    }
}

bool TModel::HasVariable(const TString& name) const {
    return AllVariablesSet.contains(name);
}

void TModel::RenameVariable(const TString& name, const TString& newName) {
    Y_VERIFY(HasVariable(name), "%s", name.c_str());
    Y_VERIFY(!HasVariable(newName), "%s", name.c_str());
    Parameters.RenameVariable(name, newName);
    for (auto& layer : Layers) {
        layer->RenameVariable(name, newName);
    }
    for (auto& v : Inputs) {
        TryRename(name, newName, v);
    }
    AllVariablesSet.erase(name);
    AllVariablesSet.insert(newName);
}

void TModel::RemoveVariable(const TString& name) {
    TVector<TString> allVariables = AllVariables();
    Y_ENSURE(Find(allVariables.begin(), allVariables.end(), name) == allVariables.end(),
        "Cannot remove variable " + name + ", which is still used by model layers");
    Parameters.RemoveVariable(name);
}

TString TModel::ModelGraphString() const {
    TStringStream ss;
    for (const auto& layer : Layers) {
        TVector<TString> inputs = layer->GetInputs();
        TVector<TString> outputs = layer->GetOutputs();
        ss << JoinStrings(outputs, ",") << " = " << layer->GetName() <<
            "(" << JoinStrings(inputs, ",") << ")" << Endl;
    }
    ss << "Inputs:\n";
    ss << JoinStrings(Inputs, ",") << Endl;
    ss << "Parameters:\n";
    ss << JoinStrings(Parameters.GetVariables(), ",") << Endl;
    return TString(ss.Data(), ss.Size());
}

static TString Sanitize(TString s) {
    SubstGlobal(s, "$", "_");
    return s;
}

TString TModel::ModelGraphDotString() const {
    TStringStream ss;
    ss << "digraph model {" << Endl;

    for (const auto& i: Inputs) {
        ss << Sanitize(i) << " [label=\"" << EscapeC(i) << "\", shape=ellipse, style=bold];" << Endl;
    }

    for (const auto& p: Parameters.GetVariables()) {
        ss << Sanitize(p) << " [label=\"" << EscapeC(p) << "\", shape=ellipse, style=filled, fillcolor=gray];" << Endl;
    }

    for (size_t l = 0; l < Layers.size(); ++l) {
        const auto& layer = Layers[l];
        TString layerId = "layer_" + IntToString<10>(l);
        ss << layerId << " [shape=box, label=\"" << EscapeC(layer->GetName()) << "\"];" << Endl;

        for (const auto& i: layer->GetInputs()) {
            ss << Sanitize(i) << " -> " << layerId << ";" << Endl;
        }

        for (const auto& o: layer->GetOutputs()) {
            ss << Sanitize(o) << " [label=\"" << EscapeC(o) << "\", shape=ellipse, style=dashed];" << Endl;
            ss << layerId << " -> " << Sanitize(o) << ";" << Endl;
        }
    }
    ss << "}" << Endl;

    return TString(ss.Data(), ss.Size());
}

namespace {

    using TNameToLayer = THashMap<TString, ILayerPtr>;

    //! Build a map of outputs to layers.
    TNameToLayer MakeOutputMap(const TModel& model) {
        TNameToLayer result;
        for (const ILayerPtr& layerPtr : model.Layers) {
            Y_ASSERT(layerPtr != nullptr);
            for (auto& name : layerPtr->GetOutputs()) {
                result.emplace(name, layerPtr);
            }
        }
        return result;
    }
} // unnamed namespace

std::pair<TVector<ILayerPtr>, TVector<TString>>
GetOutputDependencies(const TModel& model, const TString& outputName,
    const TSet<TString>& terminalInputs)
{
    return GetOutputDependencies(model, TSet<TString>{ outputName }, terminalInputs);
}

std::pair<TVector<ILayerPtr>, TVector<TString>>
GetOutputDependencies(const TModel& model, const TSet<TString>& outputsNames,
    const TSet<TString>& terminalInputs, THashSet<TString>* usefulVariablesCopy)
{
    auto outputToLayer = MakeOutputMap(model);
    for (const TString& outputName : outputsNames) {
        Y_VERIFY(outputToLayer.count(outputName) != 0,
            "Model does not have specified output");
    }

    TVector<ILayerPtr> layers;
    TVector<TString> freeNames;

    THashSet<TString> usefulVariables;
    usefulVariables.insert(outputsNames.begin(), outputsNames.end());
    for (size_t i = model.Layers.size(); i-- > 0;) {
        auto layer = model.Layers[i];
        for (const auto& output : layer->GetOutputs()) {
            if (!terminalInputs.contains(output) && usefulVariables.contains(output)) {
                layers.push_back(layer);
                for (const auto& input : layer->GetInputs()) {
                    if (outputToLayer.contains(input)) {
                        usefulVariables.insert(input);
                    } else {
                        freeNames.push_back(input);
                    }
                }
                break;
            }
        }
    }
    if (usefulVariablesCopy) {
        *usefulVariablesCopy = usefulVariables;
    }

    // restore the right order
    Reverse(layers.begin(), layers.end());
    TVector<ILayerPtr> uniqLayers;
    for (const auto& layer : layers) {
        bool isUniq = false;
        for (const auto& output : layer->GetOutputs()) {
            if (usefulVariables.contains(output)) {
                usefulVariables.erase(output);
                isUniq = true;
            }
        }
        if (isUniq) {
            uniqLayers.push_back(layer);
        }
    }

    // get rid of repeated entries
    Sort(freeNames.begin(), freeNames.end());
    auto uniqueEnd = Unique(freeNames.begin(), freeNames.end());
    freeNames.erase(uniqueEnd, freeNames.end());
    return std::make_pair(uniqLayers, freeNames);
}

TContext ExtractSubcontextFromModel(const TModel& model, const TVector<TString>& names)
{
    auto context = TContext{};
    for (const auto& name : names) {
        //Y_VERIFY(model.Parameters.has(name),
        //    "Model does not have required context data");
        context[name] = model.Parameters.at(name);
    }
    return context;
}

TString GetModelInputName(const NNeuralNetApplier::TModel& model) {
    Y_VERIFY(model.Inputs.size() == 1);
    return model.Inputs[0];
}

std::pair<TModel, TContext>
ExtractSubmodelData(const TModel& model, const TString& outputName,
        const TSet<TString>& terminalInputs)
{
    TModel res = *model.GetSubmodel(outputName, terminalInputs);
    return std::make_pair(res, res.Parameters);
}

TModelPtr TModel::GetSubmodel(const TString& outputName, const TSet<TString>& terminalInputs) const {
    return GetSubmodel(TSet<TString>{ outputName }, terminalInputs);
}

TModelPtr TModel::GetSubmodel(const TSet<TString>& outputsNames, const TSet<TString>& terminalInputs) const {
    THashSet<TString> usefulVariables;
    auto layersAndFreeVariables = GetOutputDependencies(*this, outputsNames, terminalInputs, &usefulVariables);
    TModelPtr submodel = new TModel{};
    submodel->SetSupportedVersions(GetSupportedVersions());
    submodel->Blob = Blob;
    for (auto& layer : layersAndFreeVariables.first) {
        submodel->Layers.push_back(layer->CloneForOutput(usefulVariables));
    }

    TVector<TString> parameters;
    for (const auto& var : layersAndFreeVariables.second) {
        if (!terminalInputs.contains(var) && Find(Inputs.begin(), Inputs.end(), var) == Inputs.end()) {
            parameters.push_back(var);
        } else {
            submodel->Inputs.push_back(var);
        }
    }
    submodel->Parameters = ExtractSubcontextFromModel(*this, parameters);
    for (const auto& input : Inputs) {
        if (submodel->Parameters.has(input) && !terminalInputs.contains(input)) {
            throw yexception() << "Input '" << input << "' is terminal, but was not listed";
        }
    }
    for (const auto& var : submodel->Inputs) {
        if (Parameters.has(var)) {
            submodel->Parameters[var] = CreateState(Parameters.at(var)->GetTypeName());
        }
    }
    submodel->Init();
    return submodel;
}

ui32 TModel::GetVersion() const noexcept {
    return SupportedVersions.GetEnd();
}

TVersionRange TModel::GetSupportedVersions() const noexcept {
    return SupportedVersions;
}

void TModel::SetSupportedVersions(TVersionRange versions) {
    SupportedVersions = versions;
}

void TModel::IncreaseVersion(bool supportPrevious/* = false*/) {
    if (!supportPrevious) {
        SupportedVersions = TVersionRange(GetVersion() + 1);
    } else {
        SupportedVersions = TVersionRange(SupportedVersions.GetBegin(), GetVersion() + 1);
    }
}

bool TModel::SupportsVersion(ui32 version) const {
    return SupportedVersions.Contains(version);
}

const TString& TModel::GetMetaData() const {
    return MetaData;
}

void TModel::SetMetaData(const TString& data) {
    MetaData = data;
}

}  // namespace NNeuralNetApplier
