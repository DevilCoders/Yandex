#include "perceptron.h"

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/random/random.h>

#include <library/cpp/scheme/scheme.h>

namespace NNeuralNetwork {
    // TActivationFunction
    class TActivationFunction::TActivationFunctionImpl {
    private:
        TActivationFunctionImpl(const TActivationFunctionImpl&) = delete;
        TActivationFunctionImpl(TActivationFunctionImpl&&) = delete;
        TActivationFunctionImpl& operator=(const TActivationFunctionImpl&) = delete;
        TActivationFunctionImpl& operator=(TActivationFunctionImpl&&) = delete;

    protected:
        TActivationFunctionImpl() {
        }

    public:
        using TDataType = double;

        virtual ~TActivationFunctionImpl() {
        }
        virtual TDataType Calculate(const TDataType& val) const = 0;
        virtual TDataType Derivative(const TDataType& val) const = 0;
        virtual NSc::TValue ToScheme() const = 0;
    };

    class TSigmoid: public TActivationFunction::TActivationFunctionImpl {
    private:
        TDataType Alpha;

    public:
        TSigmoid(const TDataType& alpha)
            : Alpha(alpha)
        {
        }
        TDataType Calculate(const TDataType& value) const override {
            return 1.0 / (1.0 + exp(-Alpha * value));
        }
        TDataType Derivative(const TDataType& value) const override {
            return Alpha * value * (1 - value);
        }
        NSc::TValue ToScheme() const override {
            NSc::TValue res;
            res["type"] = "sigmoid";
            res["alpha"] = Alpha;
            return res;
        }
    };

    class TReLU: public TActivationFunction::TActivationFunctionImpl {
    private:
        TDataType Alpha; // Alpha >= 0, see Derivative()
    public:
        TReLU(const TDataType& alpha)
            : Alpha(alpha)
        {
        }
        TDataType Calculate(const TDataType& value) const override {
            return value > 0.0 ? value : Alpha * value;
        }
        TDataType Derivative(const TDataType& value) const override {
            return value > 0.0 ? 1.0 : Alpha;
        }
        NSc::TValue ToScheme() const override {
            NSc::TValue res;
            res["type"] = "relu";
            res["alpha"] = Alpha;
            return res;
        }
    };

    class TTrivialActivation: public TActivationFunction::TActivationFunctionImpl {
    public:
        TTrivialActivation() {
        }
        TDataType Calculate(const TDataType& value) const override {
            return value;
        }
        TDataType Derivative(const TDataType&) const override {
            return 1;
        }
        NSc::TValue ToScheme() const override {
            NSc::TValue res;
            res["type"] = "trivial";
            return res;
        }
    };

    TActivationFunction::TActivationFunction() {
    }

    TActivationFunction::TActivationFunction(TActivationFunctionImpl* impl)
        : Impl(impl)
    {
    }

    TActivationFunction::TActivationFunction(const TActivationFunction& rgt)
        : Impl(rgt.Impl)
    {
    }

    TActivationFunction& TActivationFunction::operator=(const TActivationFunction& rgt) {
        Impl = rgt.Impl;
        return *this;
    }

    TActivationFunction::~TActivationFunction() {
    }

    TActivationFunction::TDataType TActivationFunction::operator()(const TDataType& val) const {
        return Impl->Calculate(val);
    }

    TActivationFunction::TDataType TActivationFunction::Derivative(const TDataType& val) const {
        return Impl->Derivative(val);
    }

    NSc::TValue TActivationFunction::ToScheme() const {
        return Impl->ToScheme();
    }

    TActivationFunction TActivationFunction::FromScheme(NSc::TValue scheme) {
        TActivationFunction res;
        const auto& type = scheme["type"].GetString();
        if (type == "sigmoid")
            res.Impl.Reset(new TSigmoid(scheme["alpha"].GetNumber()));
        else if (type == "relu")
            res.Impl.Reset(new TReLU(scheme["alpha"].GetNumber()));
        else if (type == "trivial")
            res.Impl.Reset(new TTrivialActivation);
        else
            res.Impl.Reset(new TTrivialActivation);
        return res;
    }

    TActivationFunction TActivationFunction::CreateDefault() {
        return TActivationFunction(new TSigmoid(2));
    }

    TActivationFunction TActivationFunction::CreateSigmoid(double alpha) {
        return TActivationFunction(new TSigmoid(alpha));
    }

    TActivationFunction TActivationFunction::CreateReLU(double alpha) {
        return TActivationFunction(new TReLU(alpha));
    }

    TActivationFunction TActivationFunction::CreateTrivial() {
        return TActivationFunction(new TTrivialActivation);
    }

    // Rumelhart perceptron
    TRumelhartPerceptron::TRumelhartPerceptron() {
    }

    TRumelhartPerceptron::TRumelhartPerceptron(double maxInit, const TVector<size_t>& dimensions) {
        size_t n = dimensions.size();
        Layers.reserve(n - 1);
        Sigmas.resize(n, TActivationFunction::CreateDefault());
        for (size_t k = 0; k + 1 < n; ++k) {
            Layers.push_back(TLayerMatrix(dimensions[k + 1], TVectorType(dimensions[k])));
            TLayerMatrix& w = Layers.back();
            for (size_t i = 0; i < dimensions[k + 1]; ++i) {
                for (size_t j = 0; j < dimensions[k]; ++j) {
                    w[i][j] = (RandomNumber<double>() - 0.5) * 2 * maxInit; // Fill with random numbers
                }
            }
        }
    }

    NSc::TValue TRumelhartPerceptron::ToScheme() const {
        NSc::TValue res;
        NSc::TValue sigmas = res["sigmas"].SetArray();
        for (const auto& sigma : Sigmas)
            sigmas.Push(sigma.ToScheme());
        res["layers"] = NJsonConverters::ToTValue(Layers);
        return res;
    }

    bool TRumelhartPerceptron::FromScheme(const NSc::TValue& scheme) {
        Sigmas.clear();
        NSc::TValue sigmas = scheme["sigmas"];
        for (size_t idx = 0; sigmas.Has(idx); ++idx)
            Sigmas.push_back(TActivationFunction::FromScheme(sigmas.Get(idx)));
        NJsonConverters::FromTValue(scheme["layers"], Layers, true);
        return true;
    }

    TString TRumelhartPerceptron::SaveAsString() const {
        NSc::TValue scheme = ToScheme();
        return scheme.ToJson();
    }

    bool TRumelhartPerceptron::LoadFromString(const TString& data) {
        try {
            NSc::TValue scheme = NSc::TValue::FromJson(data);
            FromScheme(scheme);
        } catch (...) {
            return false;
        }
        return true;
    }

    void TRumelhartPerceptron::Calculate(const TVectorType& x, TOutputs& outputs) const {
        CalculateReduced(x, Layers.size(), outputs);
    }

    void TRumelhartPerceptron::CalculateReduced(const TVectorType& x, int k, TOutputs& outputs) const {
        DoCalculateReduced(x, k, GetLayers(), Sigmas, outputs);
    }

    void TRumelhartPerceptron::Calculate(const TVectorType& x, TVectorType& y) const {
        TOutputs outputs;
        Calculate(x, outputs);
        y = std::move(outputs.back());
    }

    void TRumelhartPerceptron::CalculateReduced(const TVectorType& x, int k, TVectorType& y) const {
        TOutputs outputs;
        CalculateReduced(x, k, outputs);
        y = std::move(outputs.back());
    }

    TRumelhartPerceptron::TOutputs TRumelhartPerceptron::CalculateAndReturnResult(const TVectorType& x) const {
        TOutputs result;
        Calculate(x, result);
        return result;
    }

    TRumelhartPerceptron::TOutputs TRumelhartPerceptron::CalculateReducedAndReturnResult(const TVectorType& x, int k) const {
        TOutputs result;
        CalculateReduced(x, k, result);
        return result;
    }

    const TRumelhartPerceptron::TLayers& TRumelhartPerceptron::GetLayers() const {
        return Layers;
    }

    void TRumelhartPerceptron::ModifyLayers(TLayers& diff) {
        for (size_t k = 0, layersCount = diff.size(); k < layersCount; ++k) {
            TLayerMatrix& w = Layers[k];
            TLayerMatrix& dw = diff[k];
            for (size_t i = 0, m = dw.size(); i < m; ++i) {
                TVectorType& row = w[i];
                TVectorType& rowDiff = dw[i];
                for (size_t j = 0, n = rowDiff.size(); j < n; ++j) {
                    row[j] += rowDiff[j];
                    rowDiff[j] = 0.0;
                }
            }
        }
    }

    const TRumelhartPerceptron::TSigmas& TRumelhartPerceptron::GetSigmas() const {
        return Sigmas;
    }

    void TRumelhartPerceptron::ReplaceSigma(size_t k, const TActivationFunction& sigma) {
        Sigmas[k] = sigma;
    }

    /*
        TBaseErrorFunction - callback for TBackPropagationLearner
                             called on last layer of back propagation algorithm
    */
    class TBaseErrorFunction {
    public:
        using TDataType = TRumelhartPerceptron::TDataType;
        using TVectorType = TRumelhartPerceptron::TVectorType;

    protected:
        TBaseErrorFunction() {
        }
        ~TBaseErrorFunction() {
        }

    private:
        TBaseErrorFunction(const TBaseErrorFunction&) = delete;
        TBaseErrorFunction& operator=(const TBaseErrorFunction&) = delete;

    public:
        virtual void Prepare(const TVectorType& /*outputs*/) {
        }
        virtual TDataType CalculateGradient(const TVectorType& outputs, size_t index) const = 0; // Calculates gradient by 'index' component
        virtual TDataType CalculateValue(const TVectorType& outputs) const = 0;                  // Value of the functional
        virtual bool IsQualityFunctional() const = 0;                                            // true if quality, false if error
    };

    // Gradient descent method for learing Rumelhart perceptron
    TBackPropagationLearner::TBackPropagationLearner() {
    }

    TBackPropagationLearner::TBackPropagationLearner(double maxInit, const TVector<size_t>& dimensions)
        : TRumelhartPerceptron(maxInit, dimensions)
    {
    }

    NSc::TValue TBackPropagationLearner::SavePerceptronAsScheme() const {
        return ToScheme();
    }

    bool TBackPropagationLearner::LoadPerceptronFromScheme(const NSc::TValue& scheme) {
        return FromScheme(scheme);
    }

    TString TBackPropagationLearner::SavePerceptronAsString() const {
        return SaveAsString();
    }

    bool TBackPropagationLearner::LoadPerceptronFromString(const TString& data) {
        try {
            LoadFromString(data);
        } catch (...) {
            return false;
        }
        return true;
    }

    void TBackPropagationLearner::DoAdd(const TOutputs& outputs, TBaseErrorFunction& error, TLayers& diff, const TLayers& nn, const TSigmas& sigmas) {
        // Calculate gradient coefficients
        size_t n1 = outputs.size();
        TOutputs deltas;
        deltas.reserve(n1);
        for (const auto& o : outputs)
            deltas.push_back(TVectorType(o.size()));
        for (size_t k = n1; k > 0; --k) {
            const TVectorType& o = outputs[k - 1];
            TVectorType& d = deltas[k - 1];
            size_t m = o.size();
            if (k == n1) { // The last (output) layer
                for (size_t i = 0; i < m; ++i) {
                    d[i] = sigmas[k - 1].Derivative(o[i]) * error.CalculateGradient(o, i);
                }
            } else { // Intermediate layers
                TVectorType& dNext = deltas[k];
                const TLayerMatrix& w = nn[k - 1];
                size_t u = dNext.size();
                for (size_t i = 0; i < m; ++i) {
                    d[i] = 0.0;
                    for (size_t t = 0; t < u; ++t) {
                        d[i] += (dNext[t] * w[t][i]);
                    }
                    d[i] *= sigmas[k - 1].Derivative(o[i]);
                }
            }
        }

        // Accumulate gradient coefficients
        for (size_t k = 0, layersCount = diff.size(); k < layersCount; ++k) {
            TLayerMatrix& dw = diff[k];
            const TVectorType& o = outputs[k];
            TVectorType& d = deltas[k + 1];
            for (size_t i = 0, m = dw.size(); i < m; ++i) {
                for (size_t j = 0, n2 = dw[i].size(); j < n2; ++j) {
                    double val = d[i] * o[j];
                    if (!error.IsQualityFunctional())
                        val = -val;
                    dw[i][j] += val;
                }
            }
        }
    }

    TBackPropagationLearner::TDataType TBackPropagationLearner::Add(const TVectorType& x, TBaseErrorFunction&& error) {
        // Init Diff matrixes if they are empty
        InitAccumulated(Diff, AdaDeltas, GetLayers());
        // Calculate current output of network
        TOutputs outputs;
        Calculate(x, outputs);
        error.Prepare(outputs.back());
        DoAdd(outputs, error, Diff, GetLayers(), GetSigmas());
        return error.CalculateValue(outputs.back());
    }

    TBackPropagationLearner::TDataType TBackPropagationLearner::GradientChecking(const TVectorType& x, const TDataType& eps, TBaseErrorFunction&& error) const {
        TLayers smoothGrad, numGrad;
        TVector<TAdaDelta> fake;
        InitAccumulated(smoothGrad, fake, GetLayers());
        fake.clear();
        InitAccumulated(numGrad, fake, GetLayers());
        // Calculate smoothed
        TOutputs outputs;
        Calculate(x, outputs);
        error.Prepare(outputs.back());
        DoAdd(outputs, error, smoothGrad, GetLayers(), GetSigmas());
        double result = 0.0;
        for (size_t k = 0, layersCount = GetLayers().size(); k < layersCount; ++k) {
            const TLayerMatrix& w = GetLayers()[k];
            for (size_t i = 0, m = w.size(); i < m; ++i) {
                for (size_t j = 0, n = w[i].size(); j < n; ++j) {
                    TLayers nn(GetLayers());
                    nn[k][i][j] = GetLayers()[k][i][j] + eps;
                    DoCalculateReduced(x, 0, nn, GetSigmas(), outputs);
                    error.Prepare(outputs.back());
                    TDataType df = error.CalculateValue(outputs.back());
                    nn[k][i][j] = GetLayers()[k][i][j] - eps;
                    DoCalculateReduced(x, 0, nn, GetSigmas(), outputs);
                    error.Prepare(outputs.back());
                    df -= error.CalculateValue(outputs.back());
                    df /= (2 * eps);
                    if (!error.IsQualityFunctional())
                        smoothGrad[k][i][j] = -smoothGrad[k][i][j];
                    numGrad[k][i][j] = df;
                    result += ((smoothGrad[k][i][j] - numGrad[k][i][j]) * (smoothGrad[k][i][j] - numGrad[k][i][j]));
                }
            }
        }
        return result;
    }

    TBackPropagationLearner::TDataType TBackPropagationLearner::FlushAdaDelta(double rho, double epsilon) {
        // Calculate shrinkage coefficients
        for (size_t k = 0, layersCount = Diff.size(); k < layersCount; ++k) {
            double g2 = 0.0;
            const TLayerMatrix& dw = Diff[k];
            for (size_t i = 0, m = dw.size(); i < m; ++i) {
                for (size_t j = 0, n = dw[i].size(); j < n; ++j) {
                    g2 += (dw[i][j] * dw[i][j]);
                }
            }
            AdaDeltas[k].Gt = rho * AdaDeltas[k].Gt + (1 - rho) * g2;
            AdaDeltas[k].RMSGt = sqrt(AdaDeltas[k].Gt + epsilon);
        }

        // Update diff matrixes
        TDataType maxDW = 0.0;
        for (size_t k = 0, layersCount = Diff.size(); k < layersCount; ++k) {
            double d2 = 0.0;
            TLayerMatrix& dw = Diff[k];
            for (size_t i = 0, m = dw.size(); i < m; ++i) {
                for (size_t j = 0, n = dw[i].size(); j < n; ++j) {
                    dw[i][j] = AdaDeltas[k].RMSDt / AdaDeltas[k].RMSGt * dw[i][j];
                    d2 += (dw[i][j] * dw[i][j]);
                    maxDW = Max(maxDW, Abs(dw[i][j]));
                }
            }
            AdaDeltas[k].Dt = rho * AdaDeltas[k].Dt + (1 - rho) * d2;
            AdaDeltas[k].RMSDt = sqrt(AdaDeltas[k].Dt + epsilon);
        }

        // Add diffs to network coefficients
        ModifyLayers(Diff);
        return maxDW;
    }

    double TBackPropagationLearner::FlushSimple(double nu) {
        // Update diff matrixes
        double maxDW = 0.0;
        for (size_t k = 0, layersCount = Diff.size(); k < layersCount; ++k) {
            TLayerMatrix& dw = Diff[k];
            for (size_t i = 0, m = dw.size(); i < m; ++i) {
                for (size_t j = 0, n = dw[i].size(); j < n; ++j) {
                    dw[i][j] *= nu;
                    maxDW = Max(maxDW, Abs(dw[i][j]));
                }
            }
        }

        // Add diffs to network coefficients
        ModifyLayers(Diff);
        return maxDW;
    }

    void TBackPropagationLearner::InitAccumulated(TLayers& diff, TVector<TAdaDelta>& adaDeltas, const TLayers& nn) {
        if (diff.empty()) {
            adaDeltas.reserve(nn.size());
            diff.reserve(nn.size());
            for (const auto& w : nn) {
                adaDeltas.push_back(TAdaDelta());
                diff.push_back(TLayerMatrix());
                TLayerMatrix& dw = diff.back();
                dw.reserve(w.size());
                for (const auto& row : w) {
                    dw.push_back(TVectorType(row.size()));
                }
            }
        }
    }

    /*
        TRMSELossFunction - \sum_i{(o[i]-y[i])^2/2}
    */
    class TRMSELossFunction: public TBaseErrorFunction {
    private:
        const TVectorType& Targets;

    public:
        TRMSELossFunction(const TVectorType& targets)
            : Targets(targets)
        {
        }
        TDataType CalculateGradient(const TVectorType& outputs, size_t index) const override {
            return outputs[index] - Targets[index];
        }
        TDataType CalculateValue(const TVectorType& outputs) const override {
            TDataType result = 0;
            for (size_t index = 0; index < outputs.size(); ++index)
                result += ((outputs[index] - Targets[index]) * (outputs[index] - Targets[index]) / 2);
            return result;
        }
        bool IsQualityFunctional() const override {
            return false;
        }
    };

    // Implementation of bp method using TRMSELossFunction
    TRMSELearner::TRMSELearner() {
    }

    TRMSELearner::TRMSELearner(double maxInit, const TVector<size_t>& dimensions)
        : TBackPropagationLearner(maxInit, dimensions)
    {
    }

    double TRMSELearner::Add(const TVectorType& x, const TVectorType& y) {
        return TBackPropagationLearner::Add(x, TRMSELossFunction(y));
    }

    double TRMSELearner::GradientChecking(const TVectorType& x, const TVectorType& y, double eps) const {
        return TBackPropagationLearner::GradientChecking(x, eps, TRMSELossFunction(y));
    }

    /*
        TRegressionLossFunction - (\sum_i{o[i]*basis[i]}-y)^2/2
    */
    class TRegressionLossFunction: public TBaseErrorFunction {
    private:
        const TVectorType& Basis;
        const TDataType& Target;

    public:
        TRegressionLossFunction(const TVectorType& basis, const TDataType& target)
            : Basis(basis)
            , Target(target)
        {
        }
        TDataType CalculateGradient(const TVectorType& outputs, size_t index) const override {
            TDataType result = -Target;
            for (size_t j = 0, n = Basis.size(); j < n; ++j) {
                result += (Basis[j] * outputs[j]);
            }
            result *= Basis[index];
            return result;
        }
        TDataType CalculateValue(const TVectorType& outputs) const override {
            TDataType result = -Target;
            for (size_t j = 0, n = Basis.size(); j < n; ++j) {
                result += (Basis[j] * outputs[j]);
            }
            result = result * result / 2;
            return result;
        }
        bool IsQualityFunctional() const override {
            return false;
        }
    };

    // Implementation of bp method using TRMSELossFunction
    TRegressionLearner::TRegressionLearner(const TVectorType& basis)
        : Basis(basis)
    {
    }

    TRegressionLearner::TRegressionLearner(const TVectorType& basis, double maxInit, const TVector<size_t>& dimensions)
        : TBackPropagationLearner(maxInit, dimensions)
        , Basis(basis)
    {
    }

    double TRegressionLearner::Add(const TVectorType& x, const TDataType& y) {
        return TBackPropagationLearner::Add(x, TRegressionLossFunction(Basis, y));
    }

    /*
        TArgmaxQualityFunction
    */
    class TArgmaxQualityFunction: public TBaseErrorFunction {
    private:
        const TVectorType& Values;
        TDataType F = 0.0;
        TDataType SF = 0.0;
        TDataType F2 = 0.0;

    public:
        TArgmaxQualityFunction(const TVectorType& values)
            : Values(values)
        {
        }
        void Prepare(const TVectorType& outputs) override {
            F = SF = F2 = 0;
            for (size_t i = 0, n = outputs.size(); i < n; ++i) {
                F += outputs[i];
                SF += (outputs[i] * Values[i]);
            }
            if (F < 1e-12) {
                F = 1.0;
            }
            F2 = F * F;
        }
        TDataType CalculateGradient(const TVectorType& /*outputs*/, size_t index) const override {
            return ((Values[index] * F - SF) / F2);
        }
        TDataType CalculateValue(const TVectorType& outputs) const override {
            TDataType result = 0;
            for (size_t index = 0; index < Values.size(); ++index) {
                result += (Values[index] * outputs[index]);
            }
            result /= SF;
            return result;
            ;
        }
        bool IsQualityFunctional() const override {
            return true;
        }
    };

    // Implementation of bp method using TArgmaxQualityFunction
    TArgmaxLearner::TArgmaxLearner() {
    }

    TArgmaxLearner::TArgmaxLearner(double maxInit, const TVector<size_t>& dimensions)
        : TBackPropagationLearner(maxInit, dimensions)
    {
    }

    double TArgmaxLearner::Add(const TVectorType& x, const TVectorType& y) {
        return TBackPropagationLearner::Add(x, TArgmaxQualityFunction(y));
    }

    /*
        TSoftmaxQualityFunction
    */
    class TSoftmaxQualityFunction: public TBaseErrorFunction {
    private:
        size_t Target; // Target shown
        double Score;  // Score gain
        ui32 Mask;     // Mask of allowed classes to learn on this iteration
        TVector<TDataType> Sums;

    public:
        TSoftmaxQualityFunction(size_t target, double score, ui32 mask)
            : Target(target)
            , Score(score)
            , Mask(mask)
        {
        }
        void Prepare(const TVectorType& outputs) override {
            TVector<TDataType>(outputs.size()).swap(Sums);
            for (size_t i = 0, n = outputs.size(); i < n; ++i) {
                for (size_t j = 0; j < n; ++j) {
                    Sums[j] += exp(outputs[i] - outputs[j]);
                }
            }
        }
        TDataType CalculateGradient(const TVectorType&, size_t index) const override {
            if (((1 << index) & Mask) == 0)
                return TDataType();
            TDataType t = -1 / Sums[index] / Sums[Target];
            if (Target == index)
                t += (1 / Sums[Target]);
            return t * Score;
        }
        TDataType CalculateValue(const TVectorType&) const override {
            return Score / Sums[Target];
        }
        bool IsQualityFunctional() const override {
            return true;
        }
    };

    // Implementation of bp method using TSoftmaxQualityFunction
    TSoftmaxLearner::TSoftmaxLearner() {
    }

    TSoftmaxLearner::TSoftmaxLearner(double maxInit, const TVector<size_t>& dimensions)
        : TBackPropagationLearner(maxInit, dimensions)
    {
        size_t cnt = GetSigmas().size();
        ReplaceSigma(cnt - 1, TActivationFunction::CreateTrivial());
    }

    TSoftmaxLearner::TSoftmaxLearner(double maxInit, double alpha, const TVector<size_t>& dimensions)
        : TBackPropagationLearner(maxInit, dimensions)
    {
        size_t cnt = GetSigmas().size();
        for (size_t i = 1; i + 1 < cnt; ++i)
            ReplaceSigma(i, TActivationFunction::CreateReLU(alpha));
        ReplaceSigma(0, TActivationFunction::CreateTrivial());
        ReplaceSigma(cnt - 1, TActivationFunction::CreateTrivial());
    }

    double TSoftmaxLearner::Add(const TVectorType& x, size_t target, double score, i32 mask) {
        return TBackPropagationLearner::Add(x, TSoftmaxQualityFunction(target, score, static_cast<ui32>(mask)));
    }

    double TSoftmaxLearner::GradientChecking(const TVectorType& x, size_t target, double score, double eps) const {
        return TBackPropagationLearner::GradientChecking(x, eps, TSoftmaxQualityFunction(target, score, static_cast<ui32>(-1)));
    }

}
