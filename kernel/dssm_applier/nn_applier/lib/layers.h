#pragma once

#include "parse.h"
#include "tokenizer.h"
#include "states.h"
#include "load_params.h"
#include "util.h"

#include <library/cpp/containers/comptrie/comptrie.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <util/charset/wide.h>
#include <util/memory/blob.h>
#include <util/ysaveload.h>

#include <cmath>

namespace NNeuralNetApplier {

class ILayer : public TNonCopyable {
public:
    virtual ~ILayer() = default;
    virtual void Init(const TContext& context) = 0;
    virtual void Apply(TEvalContext& context) const = 0;
    virtual void Save(IOutputStream* stream) const = 0;
    virtual void Load(TBlob& blob) = 0;
    virtual TVector<TString> GetInputs() const = 0;
    virtual TVector<TString> GetOutputs() const = 0;
    virtual TString GetTypeName() const = 0;
    virtual TString GetName() const {
        return GetTypeName();
    }

    //returns an uninitialized copy
    TAtomicSharedPtr<ILayer> Clone() const;
    virtual TAtomicSharedPtr<ILayer> CloneForOutput(const THashSet<TString>& /*usefulVariables*/) const {
        return Clone();
    }

    virtual void RenameVariable(const TString& name, const TString& newName) = 0;
};
using ILayerPtr = TAtomicSharedPtr<ILayer>;

ILayerPtr CreateLayer(const TString& layerType);

enum class EFunction {
    Rlu = 0,
    Elu = 1,
    Tanh = 2,
    Sigmoid = 3,
    Linear = 4,
    Softplus = 5,
    Selu = 6
};

struct TRlu {
    template <class T>
    static T Fprop(T input) {
        if (input > 0) {
            return input;
        } else {
            return 0;
        }
    }

    template <class T>
    static T Bprop(T input) {
        if (input > 0) {
            return 1.0;
        } else {
            return 0;
        }
    }

    static TString GetName() {
        return "rlu";
    }
};

struct TElu {
    template <class T>
    static T Fprop(T input) {
        if (input >= 0) {
            return input;
        } else {
            return exp(input) - 1;
        }
    }

    static TString GetName() {
        return "elu";
    }
};

struct TSelu {
    template <class T>
    static T Fprop(const T& input) {
        static const float ALPHA = 1.673;
        static const float SCALE = 1.051;
        if (input >= 0) {
            return SCALE * input;
        } else {
            return SCALE * (ALPHA * exp(input) - ALPHA);
        }
    }

    static TString GetName() {
        return "selu";
    }
};

struct TTanh {
    template <class T>
    static T Fprop(T input) {
        return 2 / (1 + exp(-2 * input)) - 1;
    }

    static TString GetName() {
        return "tanh";
    }
};

struct TSigmoid {
    template <class T>
    static T Fprop(T input) {
        return 1 / (1 + exp(-input));
    }

    static TString GetName() {
        return "sigmoid";
    }
};

struct TLinear {
    template <class T>
    static T Fprop(T input) {
        return input;
    }

    static TString GetName() {
        return "linear";
    }
};

struct TSoftplus {
    template <class T>
    static T Fprop(T input) {
        if (Y_UNLIKELY(input > 700)) {
            return input;
        } else {
            return log(1 + exp(static_cast<double>(input)));
        }
    }

    static TString GetName() {
        return "softplus";
    }
};

struct TConst1 {
    template <class T>
    static T Fprop(T) {
        return 1;
    }

    static TString GetName() {
        return TString("const_1");
    }
};

template <class TFunc>
class TElementwiseTransform : public ILayer {
private:
    TString Input;
    TString Output;

public:
    TElementwiseTransform() = default;
    TElementwiseTransform(const TString& input, const TString& output)
        : Input(input)
        , Output(output)
    {
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;

    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TElementwiseTransform<" + TFunc::GetName() + ">";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TColumnElemwiseTransformLayer : public ILayer {
private:
    TString Input;
    TString Output;
    TVector<EFunction> Funcs;

public:
    TColumnElemwiseTransformLayer() = default;
    TColumnElemwiseTransformLayer(const TString& input, const TString& output,
        const TVector<EFunction>& funcs
    )
        : Input(input)
        , Output(output)
        , Funcs(funcs)
    {
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;

    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TColumnElemwiseTransformLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;

    const TVector<EFunction>& GetFunctions() const {
        return Funcs;
    }
};

// Input: TSamplesVector
// Outputs: TTextsVector's
class TFieldExtractorLayer : public ILayer {
private:
    TString Input;
    THashMap<TString, TString> AnnotationToOutput;

public:
    TFieldExtractorLayer() = default;
    TFieldExtractorLayer(const TString& input,
        const THashMap<TString, TString>& annotationToOutput);
    TFieldExtractorLayer(const TString& input, const TString& inputField,
        const TString& outputField);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override;
    TVector<TString> GetOutputs() const override;

    void Init(const TContext& context) override;

    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TFieldExtractorLayer";
    }

    TString GetName() const override;

    const THashMap<TString, TString>& GetAnnotations() {
        return AnnotationToOutput;
    }

    void RenameVariable(const TString& name, const TString& newName) override;

    bool RenameField(const TString& name, const TString& newName);

private:
    void ExtractFieldsFromSample(const TAtomicSharedPtr<ISample>& sample, size_t sampleId,
        THashMap<TString, TTextsVector*>& annotationToTextVector) const;
};

class TStringToSparseMatrixLayer : public ILayer {
private:
    TString TextsInput;
    TString SparsifierInput;
    TString Output;

    TSparsifier* Sparsifier;

public:
    TStringToSparseMatrixLayer() = default;
    TStringToSparseMatrixLayer(const TString& textsInput,
        const TString& sparsifierInput,
        const TString& output);

    TVector<TString> GetInputs() const override {
        return {TextsInput, SparsifierInput};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    TSparsifier* GetSparsifier() {
        return Sparsifier;
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TStringToSparseMatrixLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TGlobalStringToSparseMatrixLayer : public ILayer {
private:
    TString TextsInput;
    TString SparsifierInput;
    TVector<TString> Output;

    TGlobalSparsifier* Sparsifier;
    TVector<size_t> OutputIndices;

public:
    TGlobalStringToSparseMatrixLayer() = default;
    TGlobalStringToSparseMatrixLayer(const TString& textsInput,
        const TString& sparsifierInput,
        const TVector<TString>& output);

    TVector<TString> GetInputs() const override {
        return {TextsInput, SparsifierInput};
    }

    TVector<TString> GetOutputs() const override {
        return Output;
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TGlobalStringToSparseMatrixLayer";
    }

    TAtomicSharedPtr<ILayer> CloneForOutput(const THashSet<TString>& usefulVariables) const override;
    void RenameVariable(const TString& name, const TString& newName) override;
};

class TSparseScaleLayer : public ILayer {
private:
    TString Input;
    TString Scaler;
    TString Output;

    TMatrix* ScalerPtr;

public:
    TSparseScaleLayer() = default;
    TSparseScaleLayer(const TString& input, const TString& scalerInput,
        const TString& output);

    TVector<TString> GetInputs() const override {
        return{ Input, Scaler };
    }

    TVector<TString> GetOutputs() const override {
        return{ Output };
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TSparseScaleLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TSparseMatrixToEmbeddingLayer : public ILayer {
private:
    TString SparseMatrixInput;
    TString EmbeddingMatrixInput;
    TString Output;

    TMatrix* EmbeddingMatrix;
    TCharMatrix* CharEmbeddingMatrix;

public:
    TSparseMatrixToEmbeddingLayer() = default;
    TSparseMatrixToEmbeddingLayer(
        const TString& sparseMatrixInput,
        const TString& embeddingMatrixInput,
        const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {SparseMatrixInput, EmbeddingMatrixInput};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    TMatrix* GetEmbeddingMatrix() {
        return EmbeddingMatrix;
    }

    TCharMatrix* GetCharEmbeddingMatrix() {
        return CharEmbeddingMatrix;
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TSparseMatrixToEmbeddingLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TVectorParseLayer : public ILayer {
private:
    TString Input;
    TString Output;
    ui64 Size;
    char Separator;

public:
    TVectorParseLayer() = default;
    TVectorParseLayer(const TString& input,
        const TString& output,
        size_t size,
        char sep);

    TVector<TString> GetInputs() const override {
        return {Input};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TVectorParseLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

template<class FormatParser>
class TRepeatedParserLayer : public ILayer {
private:
    TString Input;
    TVector<TString> OutputFields;
    TVector<TString> OutputNames;
    TString GroupsSizesName;

public:
    TRepeatedParserLayer() = default;
    TRepeatedParserLayer(const TString& input, const TVector<TString>& outputFields,
        const TVector<TString>& outputNames, const TString& groupsSizesName)
        : Input(input)
        , OutputFields(outputFields)
        , OutputNames(outputNames)
        , GroupsSizesName(groupsSizesName)
    {
    }

    TVector<TString> GetInputs() const override {
        return {Input};
    }

    TVector<TString> GetOutputs() const override {
        TVector<TString> result = {GroupsSizesName};
        result.insert(result.end(), OutputNames.begin(), OutputNames.end());
        return result;
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return TString::Join("TRepeated", FormatParser::GetFormatName(), "ParserLayer");
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TMaxPoolingLayer : public ILayer {
private:
    TString Input;
    TString InputGroupsSizes;
    TString Output;
public:
    TMaxPoolingLayer() = default;

    TMaxPoolingLayer(const TString& input, const TString& inputGroupsSizes,
        const TString& output);

    TVector<TString> GetInputs() const override {
        return {Input, InputGroupsSizes};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TMaxPoolingLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TSumPoolingLayer : public ILayer {
private:
    TString Input;
    TString InputGroupsSizes;
    TString BiasesStr;
    TString WeightsStr;
    TString Output;
public:
    TSumPoolingLayer() = default;

    TSumPoolingLayer(const TString& input, const TString& inputGroupsSizes, const TString& biases,
        const TString& weights, const TString& output);

    TVector<TString> GetInputs() const override {
        TVector<TString> r = {Input, InputGroupsSizes, BiasesStr};
        if (WeightsStr) {
            r.push_back(WeightsStr);
        }
        return r;
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TSumPoolingLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TNormalizeRowsLayer : public ILayer {
private:
    TString Input;
    TString Output;
    double NormBase;

public:
    TNormalizeRowsLayer() = default;
    TNormalizeRowsLayer(const TString& input, const TString& output, double normBase);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TNormalizeRowsLayer";
    }

    double GetNormBase() const {
        return NormBase;
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

// Use DoBug = true to support old models, which applied incorrect scaling of values
class TLayerNorm : public ILayer {
private:
    TString Input;
    TString Output;
    bool DoBug;

public:
    TLayerNorm() = default;
    TLayerNorm(const TString& input, const TString& output, bool doBug=false);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return{ Input };
    }

    TVector<TString> GetOutputs() const override {
        return{ Output };
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TLayerNorm";
    }

    TString GetName() const override {
        if (DoBug) {
            return "TLayerNorm<buggy>";
        } else {
            return "TLayerNorm";
        }
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

// Use DoBug = true to support old models, which applied incorrect scaling of values
class TLayerNorm2 : public ILayer {
private:
    TString Input;
    TString Output;

    bool DoBug;
    bool WithMean;

    bool DefaultDoBug = false;
public:
    TLayerNorm2()
        : DoBug(false)
        , WithMean(true)
    {}

    TLayerNorm2(const TString& input, const TString& output, bool doBug=false, bool withMean=true);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TLayerNorm2Fixed";
    }

    TString GetName() const override {
        return TString("TLayerNorm2") + (WithMean ? "" : "NoMean") + (DoBug ? "<buggy>" : "");
    }

    // Default for loading, if doBug has not been saved (old versions)
    void SetDefaultDoBug(bool doBug) {
        DefaultDoBug = doBug;
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TDotLayer : public ILayer {
private:
    TString Input1;
    TString Input2;
    TString Output;

public:
    TDotLayer() = default;
    TDotLayer(const TString& input1, const TString& input2, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input1, Input2};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TDotLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TConcatLayer : public ILayer {
private:
    TVector<TString> Inputs;
    TString Output;

public:
    TConcatLayer() = default;
    TConcatLayer(const TVector<TString>& inputs, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return Inputs;
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TConcatLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TSplitLayer : public ILayer {
private:
    TString Input;
    TVector<TString> Templates;
    TVector<TString> Outputs;

public:
    TSplitLayer() = default;
    TSplitLayer(const TString& input, const TVector<TString>& templates, const TVector<TString>& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        TVector<TString> tpls{Input};
        tpls.insert(tpls.end(), Templates.begin(), Templates.end());
        return tpls;
    }

    TVector<TString> GetOutputs() const override {
        return Outputs;
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TSplitLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TAddLayer : public ILayer {
private:
    TVector<TString> Inputs;
    TString Output;

public:
    TAddLayer() = default;
    TAddLayer(const TVector<TString>& inputs, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return Inputs;
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TAddLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TWeightedAddLayer : public ILayer {
private:
    TVector<TString> Inputs;
    TString Weights;
    TString Output;

public:
    TWeightedAddLayer() = default;
    TWeightedAddLayer(const TVector<TString>& inputs, const TString& weights, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        TVector<TString> res = Inputs;
        res.push_back(Weights);
        return res;
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TWeightedAddLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TMultiCosLayer : public ILayer {
private:
    TString Input1;
    TString Input2;
    TString WeightMatrixInput;
    TString BiasMatrixInput;
    TString Output;

    TMatrix* WeightMatrix;
    TMatrix* BiasMatrix;
    float Base = 1;

public:
    TMultiCosLayer() = default;
    TMultiCosLayer(const TString& input1, const TString& input2,
        const TString& weightMatrixInput, const TString& biasMatrixInput,
        const TString& output, float base);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input1, Input2, WeightMatrixInput, BiasMatrixInput};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TMultiCosLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TAffineLayer : public ILayer {
private:
    TString Input;
    TString TransfromMatrixInput;
    TString BiasMatrixInput;
    TString Output;

    TMatrix* TransfromMatrix;
    TMatrix* BiasMatrix;

public:
    TAffineLayer() = default;
    TAffineLayer(const TString& input, const TString& transformMatrixInput,
        const TString& biasMatrixInput, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, TransfromMatrixInput, BiasMatrixInput};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TAffineLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TScaleLayer : public ILayer {
private:
    TString Input;
    TString Multiplier;
    TString Bias;
    TString Output;

    float* MultiplierScalar = nullptr;
    float* BiasScalar = nullptr;

public:
    TScaleLayer() = default;
    TScaleLayer(const TString& input, const TString& multiplier,
        const TString& bias, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, Multiplier, Bias};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TScaleLayer";
    }

    TString GetName() const override {
        if (BiasScalar && MultiplierScalar) {
            return TStringBuilder() << "TScaleLayer<Multiplier = " << ToString(*MultiplierScalar) << ", Bias = " << *BiasScalar << ">";
        }
        return "TScaleLayer";
    }

    TString GetInputName() const {
        return Input;
    }

    float GetBias() const {
        return BiasScalar ? *BiasScalar : 0.0f;
    }

    float GetMultiplier() const {
        return *MultiplierScalar;
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

// Apply bias and scaling before dot product.
class TPreDotScaleLayer : public ILayer {
private:
    TString Input;
    TString Multiplier;
    TString Bias;
    TString Output;
    float BiasMultiplier = 1.0;

public:
    TPreDotScaleLayer() = default;
    TPreDotScaleLayer(const TString& input, const TString& multiplier,
                      const TString& bias, const TString& output);
    TPreDotScaleLayer(const TString& input, const TString& multiplier,
                      const TString& bias, float biasMultiplier,
                      const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override;

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetName() const override {
        return "TPreDotScaleLayer";
    }

    TString GetTypeName() const override {
        return "TPreDotScaleLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TShallowCopyLayer: public ILayer {
private:
    TString Input;
    TString Output;

public:
    TShallowCopyLayer() = default;
    TShallowCopyLayer(const TString& input, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TShallowCopyLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TAddRowLayer : public ILayer {
private:
    TString Input;
    TString AddVectorStr;
    TString Output;

    TMatrix* AddVector = nullptr;
public:
    TAddRowLayer() = default;

    TAddRowLayer(const TString& input,const TString& addVectorStr,
            const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, AddVectorStr};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TAddRowLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TOuterScaleLayer : public ILayer {
private:
    TString Input;
    TString Alpha;
    TString Beta;
    TString Output;
public:
    TOuterScaleLayer() = default;

    TOuterScaleLayer(const TString& input, const TString& alpha, const TString& beta,
        const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, Alpha, Beta};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TOuterScaleLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TSparseRescalerLayer : public ILayer {
private:
    TString Input;
    TString Parameters;
    TString Output;
public:
    TSparseRescalerLayer() = default;

    TSparseRescalerLayer(const TString& input, const TString& parameters,
        const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, Parameters};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TSparseRescalerLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TMulRowLayer : public ILayer {
private:
    TString Input;
    TString MulVectorStr;
    TString Output;

    TMatrix* MulVector;

public:
    TMulRowLayer() = default;
    TMulRowLayer(const TString& input, const TString& addVectorStr,
        const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, MulVectorStr};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TMulRowLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TAddMulLayer : public ILayer {
private:
    TString Input;
    TString AddVectorInput;
    TString MulVectorInput;
    TString Output;

    TMatrix* AddVector;
    TMatrix* MulVector;

public:
    TAddMulLayer() = default;
    TAddMulLayer(const TString& input, const TString& addVectorInput,
        const TString& mulVectorInput, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, AddVectorInput, MulVectorInput};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TAddMulLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TMulAddLayer : public ILayer {
private:
    TString Input;
    TString MulVectorInput;
    TString AddVectorInput;
    TString Output;

    TMatrix* MulVector;
    TMatrix* AddVector;

public:
    TMulAddLayer() = default;
    TMulAddLayer(const TString& input, const TString& mulVectorInput,
        const TString& addVectorInput, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, MulVectorInput, AddVectorInput};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TMulAddLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

// bias + scale * x / (constant + |x|) transformation.
class TSoftSignLayer : public ILayer {
private:
    TString Input;
    TString Output;
    float Constant;
    float Scale;
    float Bias;

private:
    float SoftSign(float x) const;

public:
    TSoftSignLayer();
    TSoftSignLayer(const TString& input, const TString& output, float constant,
        float scale, float bias);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input};
    }
    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TSoftSignLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TRemapLayer : public ILayer {
private:
    TString Input;
    TString Output;
    TVector<TVector<float>> RemapFrom;
    TVector<TVector<float>> RemapTo;

private:
    float Map(float x, ui64 idx = 0) const;

public:
    TRemapLayer();
    TRemapLayer(const TString& input, const TString& output, TVector<TVector<float>> remapFrom, TVector<TVector<float>> remapTo);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input};
    }
    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TRemapLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TZeroLayer : public ILayer {
private:
    TString Input;
    TString Output;

public:
    TZeroLayer();
    TZeroLayer(const TString& input, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input};
    }
    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TZeroLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TGroupSoftmaxLayer : public ILayer {
private:
    TString Input;
    TString Output;
    ui64 GroupSize;

public:
    TGroupSoftmaxLayer();
    TGroupSoftmaxLayer(const TString& input, const TString& output, size_t groupSize);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return{ Input };
    }
    TVector<TString> GetOutputs() const override {
        return{ Output };
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TGroupSoftmaxLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TQuantizeLayer : public ILayer {
private:
    TString Input;
    TString Output;
    ui64 NumBins;

public:
    TQuantizeLayer();
    TQuantizeLayer(const TString& input, const TString& output, size_t numBins);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return { Input };
    }
    TVector<TString> GetOutputs() const override {
        return { Output };
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetName() const override {
        return "TQuantizeLayer<" + ToString(NumBins) + ">";
    }

    TString GetTypeName() const override {
        return "TQuantizeLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TBitVectorWriter {
    ui8* Data;
    size_t Offset;
public:
    TBitVectorWriter(ui8* data)
        : Data(data)
        , Offset(0)
    {}

    // Write the lower numBits of val to Data
    void Write(ui8 val, size_t numBits);

    ui8* GetData() {
        return Data;
    }
};

class TBitVectorReader {
    const ui8* Data;
    size_t Offset = 0;
public:
    TBitVectorReader(const ui8* data)
        : Data(data)
    {}

    // Reads the lower numBits of val to Data
    ui8 Get(size_t numBits);

    const ui8* GetData() {
        return Data;
    }
};

// Takes values in the range [min,max], quantizes them into NumBins, and returns a compressed char matrix
class TCompressorLayer : public ILayer {
private:
    TString Input;
    TString Output;
    ui64 NumBins;
    ui64 NumBits;
    double Min;
    double Max;

public:
    TCompressorLayer();
    // numBits may be greater than requried to encode numBins in case we want to fill
    // the number with leading zeroes (like numBits=8 for fast integer multiplication)
    TCompressorLayer(const TString& input, const TString& output, size_t numBins,
        double min=-1, double max=1, size_t numBits=0);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return { Input };
    }
    TVector<TString> GetOutputs() const override {
        return { Output };
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetName() const override {
        return "TCompressorLayer<" + ToString(NumBins) + "," + ToString(Min) + "," + ToString(Max) + ">";
    }

    TString GetTypeName() const override {
        return "TCompressorLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

// maps data compressed by TCompressorLayer back into range [min,max]
class TDecompressorLayer : public ILayer {
private:
    TString Input;
    TString Output;
    ui64 NumBins;
    ui64 NumBits;
    double Min;
    double Max;

public:
    TDecompressorLayer();
    TDecompressorLayer(const TString& input, const TString& output,
        size_t numBins, double min, double max, size_t numBits = 0);

    static float constexpr CalcCoef(const double min, const double max, const ui64 numBins) noexcept {
        return (max - min) / (numBins - 1);
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return { Input };
    }
    TVector<TString> GetOutputs() const override {
        return { Output };
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetName() const override {
        return "TDecompressorLayer<" + ToString(NumBins) + "," + ToString(Min) + "," + ToString(Max) + ">";
    }

    TString GetTypeName() const override {
        return "TDecompressorLayer";
    }

    ui64 GetNumBins() const {
        return NumBins;
    }

    ui64 GetNumBits() const {
        return NumBits;
    }

    double GetMinValue() const {
        return Min;
    }

    double GetMaxValue() const {
        return Max;
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

// Compresses rows of one hot (like [0, 0, 1,0], group size defined by NumBins) groups,
// and returns a compressed char matrix
class TOneHotCompressorLayer : public ILayer {
private:
    TString Input;
    TString Output;
    ui64 NumBins;

public:
    TOneHotCompressorLayer();
    TOneHotCompressorLayer(const TString& input, const TString& output, size_t numBins);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return { Input };
    }
    TVector<TString> GetOutputs() const override {
        return { Output };
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetName() const override {
        return "TOneHotCompressorLayer<" + ToString(NumBins) + ">";
    }

    TString GetTypeName() const override {
        return "TOneHotCompressorLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

// Decompresses data encoded by TOneHotCompressorLayer
class TOneHotDecompressorLayer : public ILayer {
private:
    TString Input;
    TString Output;
    ui64 NumBins;

public:
    TOneHotDecompressorLayer();
    TOneHotDecompressorLayer(const TString& input, const TString& output, size_t numBins);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return { Input };
    }
    TVector<TString> GetOutputs() const override {
        return { Output };
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetName() const override {
        return "TOneHotDecompressorLayer<" + ToString(NumBins) + ">";
    }

    TString GetTypeName() const override {
        return "TOneHotDecompressorLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TSoftmaxQuantizeLayer : public ILayer {
private:
    TString Input;
    TString Output;
    ui64 NumBins;

public:
    TSoftmaxQuantizeLayer();
    TSoftmaxQuantizeLayer(const TString& input, const TString& output, size_t numBins);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return{ Input };
    }
    TVector<TString> GetOutputs() const override {
        return{ Output };
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetName() const override {
        return "TSoftmaxQuantizeLayer<" + ToString(NumBins) + ">";
    }

    TString GetTypeName() const override {
        return "TSoftmaxQuantizeLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

//multiplies each row of matrix by corresponing vector coordinate; backprops TDotLayer
class THadamarlikeProductLayer : public ILayer {
private:
    TString InputMatrix;
    TString InputVector;
    TString Output;
public:
    THadamarlikeProductLayer();
    THadamarlikeProductLayer(const TString& inputMatrix, const TString& inputVector, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {InputMatrix, InputVector};
    }
    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "THadamarlikeProductLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

//backprops TNormalizeRowsLayer
class TNormalizeRowsGradientLayer : public ILayer {
private:
    TString Input; //input of TNormalizeRow
    TString Output; //output of TNormalizeRow
    TString OutputGradient; //gradient of lower-level layer
    TString InputGradient;

    double NormBase;
public:
    TNormalizeRowsGradientLayer();
    TNormalizeRowsGradientLayer(const TString& input, const TString& output, const TString& outputGradient, const TString& inputGradient, double normBase);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, Output, OutputGradient};
    }
    TVector<TString> GetOutputs() const override {
        return {InputGradient};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TNormalizeRowsGradientLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TLinearTransposedLayer : public ILayer {
private:
    TString Input;
    TString TransfromMatrixInput;
    TString Output;

    TMatrix* TransfromMatrix;
public:
    TLinearTransposedLayer() = default;
    TLinearTransposedLayer(const TString& input, const TString& transformMatrixInput, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, TransfromMatrixInput};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TLinearTransposedLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

// Use DoBug = true to support old models, which applied incorrect scaling of values
class TNorm2GradientLayer : public ILayer {
private:
    TString Input; // Input of TLayerNorm2
    TString OutputGradient; // OutputGradient of lower-level layer
    TString InputGradient;

    bool DoBug;
public:
    TNorm2GradientLayer()
        : DoBug(false)
    {}

    TNorm2GradientLayer(const TString& input, const TString& outputGradient, const TString& inputGradient, bool doBug=false);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, OutputGradient};
    }

    TVector<TString> GetOutputs() const override {
        return {InputGradient};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TNorm2GradientLayerFixed";
    }

    TString GetName() const override {
        if (DoBug) {
            return "TNorm2GradientLayer<buggy>";
        } else {
            return "TNorm2GradientLayer";
        }
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

template<class TFunc>
class TElementwiseGradientLayer : public ILayer {
private:
    TString Input; // Input of TElementwiseLayer
    TString Output;
    TString OutputGradient;
    TString InputGradient;
public:
    TElementwiseGradientLayer() = default;

    TElementwiseGradientLayer(const TString& input, const TString& output, const TString& outputGradient, const TString& inputGradient)
        : Input(input)
        , Output(output)
        , OutputGradient(outputGradient)
        , InputGradient(inputGradient)
    {
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, Output, OutputGradient};
    }

    TVector<TString> GetOutputs() const override {
        return {InputGradient};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TElementwiseGradientLayer<" + TFunc::GetName() + ">";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TSparseMatrixToEmbeddingGradientLayer : public ILayer {
private:
    TString OutputGradient; //dense gradient
    TString SparseMatrixInput; //coordinates we are interested in
    TString EmbeddingMatrixInput; //embed matrix (which used when we made embed first)
    TString SparseMatrixInputGradient; //sparse gradient

    TMatrix* EmbeddingMatrix;
    TCharMatrix* CharEmbeddingMatrix;

public:
    TSparseMatrixToEmbeddingGradientLayer() = default;
    TSparseMatrixToEmbeddingGradientLayer(
        const TString& outputGradient,
        const TString& sparseMatrixInput,
        const TString& embeddingMatrixInput,
        const TString& sparseMatrixInputGradient);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {OutputGradient, SparseMatrixInput, EmbeddingMatrixInput};
    }

    TVector<TString> GetOutputs() const override {
        return {SparseMatrixInputGradient};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TSparseMatrixToEmbeddingGradientLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TStringToSparseMatrixAndPosLayer : public ILayer {
private:
    TString TextsInput;
    TString SparsifierInput;
    TString Output;
    TString OutputPositions;

    TSparsifier* Sparsifier;

public:
    TStringToSparseMatrixAndPosLayer() = default;
    TStringToSparseMatrixAndPosLayer(const TString& textsInput,
        const TString& sparsifierInput,
        const TString& output,
        const TString& outputPositions);

    TVector<TString> GetInputs() const override {
        return {TextsInput, SparsifierInput};
    }

    TVector<TString> GetOutputs() const override {
        return {Output, OutputPositions};
    }

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    void Init(const TContext& context) override;

    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TStringToSparseMatrixAndPosLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TJoinMultiplySparsesAndPositionsLayer : public ILayer {
private:
    TVector<TString> InputsSparse;
    TVector<TString> InputsPosition;
    TString OutputSparse;
    TString OutputPositions;

public:
    TJoinMultiplySparsesAndPositionsLayer() = default;
    TJoinMultiplySparsesAndPositionsLayer(
            const TVector<TString>& inputsSparse,
            const TVector<TString>& inputsPosition,
            const TString& outputSparse,
            const TString& outputPositions
    );

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        TVector<TString> inputs;
        inputs.insert(inputs.end(), InputsSparse.begin(), InputsSparse.end());
        inputs.insert(inputs.end(), InputsPosition.begin(), InputsPosition.end());
        return inputs;
    }

    TVector<TString> GetOutputs() const override {
        return {OutputSparse, OutputPositions};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TJoinMultiplySparsesAndPositionsLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

struct TWeightsJoinOptions {
    float WordWeight;
    //weights of trigram intersecting word by N letters
    float Trigrams3Weight;
    float Trigrams2Weight;
    float Trigrams1Weight;

    float BigramsWeight;

    void Save(IOutputStream* s) const;
    void Load(IInputStream* s);
};

class TJoinSparseGradsLayer : public ILayer {
private:
    TString Input;
    TString Positions;
    TString Output;
    TWeightsJoinOptions JoinOptions;
public:
    TJoinSparseGradsLayer() = default;
    TJoinSparseGradsLayer(const TString& input, const TString& positions, const TString& output, const TWeightsJoinOptions& options);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, Positions};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;

    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TJoinSparseGradsLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

namespace NAggregateFunctions {
struct TMax {
    const static size_t OUTPUTS = 1;

    static TVector<float> Func(const TVector<float>& values) {
        if (values.empty()) {
            return {0.0};
        }
        return {*std::max_element(values.begin(), values.end())};
    }

    static TString GetName() {
        return "max";
    }
};

struct TMin {
    const static size_t OUTPUTS = 1;

    static TVector<float> Func(const TVector<float>& values) {
        if (values.empty()) {
            return {0.0};
        }
        return {*std::min_element(values.begin(), values.end())};
    }

    static TString GetName() {
        return "min";
    }
};

struct TStd {
    const static size_t OUTPUTS = 1;

    static TVector<float> Func(const TVector<float>& values) {
        if (values.empty()) {
            return {0.0};
        }
        float mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        float norm2 = 0.0f;
        for (float x : values) {
            norm2 += (x - mean) * (x - mean);
        }
        return {sqrt(norm2 / values.size())};
    }

    static TString GetName() {
        return "std";
    }
};

struct TMean {
    const static size_t OUTPUTS = 1;

    static TVector<float> Func(const TVector<float>& values) {
        return {std::accumulate(values.begin(), values.end(), 0.0f) / (values.empty() ? 1.0f : float(values.size()))};
    }

    static TString GetName() {
        return "mean";
    }
};

struct TMoment3Central {
    const static size_t OUTPUTS = 1;

    static TVector<float> Func(const TVector<float>& values) {
        if (values.empty()) {
            return {0.0f};
        }
        float mean = std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
        float result = 0.0;
        for (float x : values) {
            result += (x - mean) * (x - mean) * (x - mean);
        }
        return {result / values.size()};
    }

    static TString GetName() {
        return "moment3_central";
    }
};
}

template<class TAggregateFunc>
class TSparseMatrixStatisticLayer : public ILayer {
private:
    TString Input;
    TString Output;
public:
    TSparseMatrixStatisticLayer() = default;
    TSparseMatrixStatisticLayer(const TString& input, const TString& output)
        : Input(input)
        , Output(output)
    {}

    void Save(IOutputStream* s) const override;

    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override {
        Y_UNUSED(context);
    }

    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TSparseMatrixStatisticLayer<" + TAggregateFunc::GetName() + ">";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TRoundToPrecisionLayer : public ILayer {
private:
    TString Input;
    TString Precision;
    TString Output;

    float* PrecisionValue = nullptr;
    float MultiplierValue;

public:
    TRoundToPrecisionLayer() = default;
    TRoundToPrecisionLayer(const TString& input, const TString& precision, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, Precision};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TRoundToPrecisionLayer";
    }

    TString GetName() const override {
        if (PrecisionValue) {
            return TStringBuilder{} << "TRoundToPrecisionLayer<Precision = " << *PrecisionValue << ">";
        }
        return "TRoundToPrecisionLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;

private:
    float RoundToPrecision(float value) const noexcept;
};

class TBatchNormLayer : public ILayer {
private:
    TString Input;
    TString MeanVectorInput;
    TString VarVectorInput;
    TString BetaVectorInput;
    TString GammaVectorInput;
    TString Output;

    TMatrix* Mean;
    TMatrix* Var;
    TMatrix* Beta;
    TMatrix* Gamma;

public:
    TBatchNormLayer() = default;
    TBatchNormLayer(
        const TString& input,
        const TString& meanVectorInput,
        const TString& varVectorInput,
        const TString& betaVectorInput,
        const TString& gammaVectorInput,
        const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input, MeanVectorInput, VarVectorInput, BetaVectorInput, GammaVectorInput};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;

    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TBatchNormLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TLinearCombinationLayer : public ILayer {
private:
    TVector<TString> Inputs;
    TString WeightsMatrixInput;
    TString Output;

    TMatrix* Weights;

public:
    TLinearCombinationLayer() = default;
    TLinearCombinationLayer(
        const TVector<TString>& inputs,
        const TString& weightsVectorInput,
        const TString& output
    );

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override;

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TLinearCombinationLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TMatMulLayer : public ILayer {
private:
    TString Input1;
    TString Input2;
    TString Output;

public:
    TMatMulLayer() = default;
    TMatMulLayer(const TString& input1, const TString& input2, const TString& output);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input1, Input2};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TMatMulLayer";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

class TParamEluLayer : public ILayer {
private:
    TString Input;
    TString Output;
    double Alpha;

public:
    TParamEluLayer() = default;
    TParamEluLayer(const TString& input, const TString& output, double alpha);

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return {Input};
    }

    TVector<TString> GetOutputs() const override {
        return {Output};
    }

    void Init(const TContext& context) override;
    void Apply(TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TParamEluLayer";
    }

    double GetAlpha() const {
        return Alpha;
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

template<class TFunc>
class TElementwiseBinaryTransform : public ILayer {
private:
    TString Input1;
    TString Input2;
    TString Output;
public:
    TElementwiseBinaryTransform() = default;
    TElementwiseBinaryTransform(const TString& input1, const TString& input2, const TString& output)
        : Input1(input1)
        , Input2(input2)
        , Output(output)
    {}

    void Save(IOutputStream* s) const override;
    void Load(TBlob& blob) override;

    TVector<TString> GetInputs() const override {
        return { Input1, Input2 };
    }

    TVector<TString> GetOutputs() const override {
        return { Output };
    }

    void Init(const NNeuralNetApplier::TContext&) override {}
    void Apply(NNeuralNetApplier::TEvalContext& context) const override;

    TString GetTypeName() const override {
        return "TElementwiseBinaryTransform<" + TFunc::GetName() + ">";
    }

    void RenameVariable(const TString& name, const TString& newName) override;
};

struct TMul {
    template<typename T>
    static T Fprop(T l, T r) {
        return l * r;
    }

    static TString GetName() {
        return "mul";
    }
};

struct TAbsDiff {
    template<typename T>
    static T Fprop(T l, T r) {
        return std::abs(l - r);
    }

    static TString GetName() {
        return "absdiff";
    }
};

class TVersionRange {
private:
    ui32 BeginRange = 0;
    ui32 EndRange = 0;

public:
    constexpr explicit TVersionRange() noexcept = default;

    constexpr explicit TVersionRange(ui32 version) noexcept
        : BeginRange(version)
        , EndRange(version)
    {
    }

    explicit TVersionRange(ui32 begin, ui32 end)
        : BeginRange(begin)
        , EndRange(end)
    {
        Y_ASSERT(EndRange >= BeginRange);
    }

    ui32 GetBegin() const noexcept {
        return BeginRange;
    }

    ui32 GetEnd() const noexcept {
        return EndRange;
    }

    bool operator==(const TVersionRange& rhs) const noexcept {
        return BeginRange == rhs.BeginRange && EndRange == rhs.EndRange;
    }

    void Save(IOutputStream* s) const {
        ::Save(s, BeginRange);
        ::Save(s, EndRange);
    }

    void Load(IInputStream* s) {
        ::Load(s, BeginRange);
        ::Load(s, EndRange);
    }

    bool Contains(ui32 version) const noexcept {
        return version >= BeginRange && version <= EndRange;
    }
};

class TModel : public TThrRefBase {
private:
    const size_t FORMAT_VERSION = 1;
    TSet<TString> AllVariablesSet;
    TVersionRange SupportedVersions;
    TString MetaData;
    TString LayersString() const;

public:
    class TInvalidFormatException: public yexception {
    public:
        TInvalidFormatException() {
            *this << "Invalid model format";
        }
    };

    TModel() = default;
    TModel(const TModel& other);
    void operator=(const TModel& other);
    static TModel FromFile(const TString& filename, bool tryLockMemory = true);

    TVector<TString> Inputs;
    TBlob Blob;
    TVector<ILayerPtr> Layers;
    TContext Parameters;

    void Init();
    void Apply(TAtomicSharedPtr<ISample> sample, TVector<float>& result) const;
    void Apply(TAtomicSharedPtr<ISample> sample, const TVector<TString>& outputVariables,
        TVector<float>& result) const;
    void Apply(const TVector<TAtomicSharedPtr<ISample>>& samples, const TVector<TString>& outputVariables,
        TVector<TVector<float>>& result) const;
    void Apply(const TVector<TAtomicSharedPtr<ISample>>& samples, const TVector<TString>& outputVariables,
        TVector<TVector<TVector<float>>>& result) const;
    void Apply(TAtomicSharedPtr<ISample> sample, const TVector<TString>& outputVariables,
        TVector<ui8>& result) const;
    void Apply(TEvalContext& evalContext, const TVector<TString>& outputVariables,
        TVector<float>& result) const;
    void Apply(TEvalContext& evalContext, const TVector<TString>& outputVariables,
               TVector<TVector<float>>& result) const;
    void Apply(TEvalContext& evalContext, const TVector<TString>& outputVariables,
               TVector<TVector<TVector<float>>>& result) const;
    void Apply(TEvalContext& evalContext) const;

    void FillEvalContextFromSample(TAtomicSharedPtr<ISample> sample, TEvalContext& evalContext) const;
    void FillEvalContextFromSamples(const TVector<TAtomicSharedPtr<ISample>>& samples, TEvalContext& evalContext) const;

    void Save(IOutputStream* s, const TVector<TString>& compressMatrixes = TVector<TString>(),
        double compressQuantile = 0, float deletionPercent = 0) const;

    void Load(const TBlob& blob, const TLoadParams& loadParams = {}); //WARNING(SPI-9321): this method try doing additional lock, which 1) break abstraction 2) may cause problems in locking from different cgroups
    void LoadNoLock(const TBlob& blob, const TLoadParams& loadParams = {});

    TIntrusivePtr<TModel> GetSubmodel(const TSet<TString>& outputsNames, const TSet<TString>& terminalInputs = {}) const;
    TIntrusivePtr<TModel> GetSubmodel(const TString& outputName, const TSet<TString>& terminalInputs = {}) const;

    void RenameVariable(const TString& name, const TString& newName);
    void RemoveVariable(const TString& name);
    bool HasVariable(const TString& name) const;

    TVector<TString> AllVariables() const;
    size_t AllVariablesCount() const;
    TString ModelGraphString() const;
    TString ModelGraphDotString() const;

    ui32 GetVersion() const noexcept;
    TVersionRange GetSupportedVersions() const noexcept;
    void SetSupportedVersions(TVersionRange range);
    void IncreaseVersion(bool supportPrevious = false);
    bool SupportsVersion(ui32 version) const;

    const TString& GetMetaData() const;
    void SetMetaData(const TString& data);
private:
    void CopyModel(const TModel& other);
    void InitAllVariablesCache();
};
using TModelPtr = TIntrusivePtr<TModel>;

// obsolete, use model.Inputs
TString GetModelInputName(const NNeuralNetApplier::TModel& model);

/** Return layers and context names that the output variable depends on.

This function finds all layers that generate intermediate and
final data required to produce the specified output. It also
ouputs names that are not names of connections between
layers (free parameters, external inputs)/
Terminal inputs are the variables that ar forced to have no parents.

The second return parameter is a list of layers inputs that are
external parameters (they should be set in the context object).
*/
std::pair<TVector<NNeuralNetApplier::ILayerPtr>, TVector<TString>>
    GetOutputDependencies(const NNeuralNetApplier::TModel& model,
        const TString& outputName,
        const TSet<TString>& terminalInputs = {});


std::pair<TVector<NNeuralNetApplier::ILayerPtr>, TVector<TString>>
    GetOutputDependencies(const NNeuralNetApplier::TModel& model,
        const TSet<TString>& outputName,
        const TSet<TString>& terminalInputs = {},
        THashSet<TString>* usefulVariablesCopy = nullptr);

// Makes a shallow copy of TContext data parameters, specified in names
NNeuralNetApplier::TContext ExtractSubcontextFromModel(const NNeuralNetApplier::TModel& model,
    const TVector<TString>& names);

/** Return context and layers required to compute specified output.

The context is returned separately and not stored in model
parameters because it may be loaded through blob and not
through stream (model is loaded through stream
object).

Terminal inputs are the variables that ar forced to have no parents.

The submodel will share layer and context data with the model
that was passed as an argument. Thus this function should mostly
be used to serialize submodel. Init function is not called for
the result.
*/
// Obsolete, use TModel::GetSubmodel
std::pair<NNeuralNetApplier::TModel, NNeuralNetApplier::TContext>
    ExtractSubmodelData(const NNeuralNetApplier::TModel& model,
        const TString& outputName,
        const TSet<TString>& terminalInputs = {});

}  // namespace NNeuralNetApplier
