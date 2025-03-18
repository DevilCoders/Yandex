#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>

namespace NSc {
    class TValue;
}

namespace NNeuralNetwork {
    /*
        TActivationFunction
    */
    class TActivationFunction {
    public:
        class TActivationFunctionImpl;

    private:
        TSimpleSharedPtr<TActivationFunctionImpl> Impl;

        TActivationFunction();
        TActivationFunction(TActivationFunctionImpl* impl);

    public:
        using TDataType = double;

    public:
        TActivationFunction(const TActivationFunction&);
        TActivationFunction& operator=(const TActivationFunction&);
        ~TActivationFunction();
        TDataType operator()(const TDataType& val) const;
        TDataType Derivative(const TDataType& val) const; // Derivative in point Sigma(x) (!)
        NSc::TValue ToScheme() const;

        static TActivationFunction FromScheme(NSc::TValue scheme);
        static TActivationFunction CreateDefault();
        static TActivationFunction CreateSigmoid(double alpha);
        static TActivationFunction CreateReLU(double alpha);
        static TActivationFunction CreateTrivial();
    };

    /*
        TRumelhartPerceptron
    */
    class TRumelhartPerceptron {
    public:
        using TDataType = double;
        using TVectorType = TVector<TDataType>;
        using TLayerMatrix = TVector<TVectorType>;
        using TLayers = TVector<TLayerMatrix>;
        using TOutputs = TVector<TVectorType>;
        using TSigmas = TVector<TActivationFunction>;

    private:
        TLayers Layers;                      // Matrixes with coefficients
        TVector<TActivationFunction> Sigmas; // Activation functions

    public:
        TRumelhartPerceptron();                                                  // Start with empty layers, use Load
        TRumelhartPerceptron(double maxInit, const TVector<size_t>& dimensions); // Start with specified layers dimensions, filled with random numbers
        NSc::TValue ToScheme() const;                                            // Save as scheme
        bool FromScheme(const NSc::TValue& scheme);                              // Load from scheme
        TString SaveAsString() const;                                            // Serialize and convert to string
        bool LoadFromString(const TString& data);                                // Deserialize from string

        void Calculate(const TVectorType& x, TOutputs& outputs) const;               // Apply, returns results for all layers (first is input, last is result)
        void CalculateReduced(const TVectorType& x, int k, TOutputs& outputs) const; // Apply, returns results for k%Layers.size()+1 layers (first is input, last is result)
        void Calculate(const TVectorType& x, TVectorType& y) const;                  // Apply, returns result for last layer
        void CalculateReduced(const TVectorType& x, int k, TVectorType& y) const;    // Apply, returns result for k-th layer
        TOutputs CalculateAndReturnResult(const TVectorType& x) const;               // Apply and return (less effective but needed by bindings)
        TOutputs CalculateReducedAndReturnResult(const TVectorType& x, int k) const; // Apply and return (less effective but needed by bindings)

    protected:
        const TLayers& GetLayers() const;                              // Access layers
        void ModifyLayers(TLayers& diff);                              // Add diff to layers, clear diff
        const TSigmas& GetSigmas() const;                              // Access activation functions
        void ReplaceSigma(size_t k, const TActivationFunction& sigma); // Replace one sigma
    };

    template <typename TVectorType, typename TLayers, typename TSigmas, typename TOutputs>
    void DoCalculateReduced(const TVectorType& x, int k, const TLayers& nn, const TSigmas& sigmas, TOutputs& outputs) {
        int n = nn.size();
        size_t m = n ? (k % n + n) % n : 0;
        if (m == 0)
            m = n;
        outputs.clear();
        outputs.reserve(m + 1);
        {
            outputs.push_back(TVectorType(x.size()));
            TVectorType& cur = outputs.back();
            for (size_t i = 0; i < x.size(); ++i)
                cur[i] = sigmas[0](x[i]);
        }
        for (size_t t = 0; t < m; ++t) {
            const auto& w = nn[t];
            TVectorType& prev = outputs.back();
            outputs.push_back(TVectorType(w.size()));
            TVectorType& next = outputs.back();
            for (size_t i = 0; i < w.size(); ++i) {
                next[i] = sigmas[t + 1](InnerProduct(w[i], prev));
            }
        }
    }

    class TBaseErrorFunction;

    /*
        TBackPropagationLearner - algorithm for learning TRumelhartPerceptron
    */
    class TBackPropagationLearner: private TRumelhartPerceptron {
    private:
        struct TAdaDelta {
            double Gt = 0.0;
            double Dt = 0.0;
            double RMSGt = 0.0;
            double RMSDt = 0.0;
        };

    private:
        TVector<TAdaDelta> AdaDeltas; // AdaDelta accumulated values
        TLayers Diff;                 // Accumulated gradient

    public:
        using TRumelhartPerceptron::TDataType;
        using TRumelhartPerceptron::TOutputs;
        using TRumelhartPerceptron::TVectorType;

    public:
        NSc::TValue SavePerceptronAsScheme() const;
        bool LoadPerceptronFromScheme(const NSc::TValue& scheme);
        TString SavePerceptronAsString() const;
        bool LoadPerceptronFromString(const TString& data);
        TDataType FlushAdaDelta(double rho, double epsilon);
        TDataType FlushSimple(double nu);

    protected:
        TBackPropagationLearner();
        TBackPropagationLearner(double maxInit, const TVector<size_t>& dimensions);
        TDataType Add(const TVectorType& x, TBaseErrorFunction&& error);                                          // Add learning sample
        TDataType GradientChecking(const TVectorType& x, const TDataType& eps, TBaseErrorFunction&& error) const; // Do gradient checking, print result to stderr
        using TRumelhartPerceptron::GetSigmas;
        using TRumelhartPerceptron::ReplaceSigma;

    private:
        static void InitAccumulated(TLayers& diff, TVector<TAdaDelta>& adaDeltas, const TLayers& nn);
        static void DoAdd(const TOutputs& outputs, TBaseErrorFunction& error, TLayers& diff,
                          const TLayers& nn, const TSigmas& sigmas); // Add learning sample
    };

    /*
        TRMSELearner
    */
    class TRMSELearner: public TBackPropagationLearner {
    public:
        TRMSELearner();
        TRMSELearner(double maxInit, const TVector<size_t>& dimensions);
        double Add(const TVectorType& x, const TVectorType& y);
        double GradientChecking(const TVectorType& x, const TVectorType& y, double eps) const;
    };

    /*
        TRegressionLearner
    */
    class TRegressionLearner: public TBackPropagationLearner {
    private:
        TVectorType Basis;

    public:
        TRegressionLearner(const TVectorType& basis);
        TRegressionLearner(const TVectorType& basis, double maxInit, const TVector<size_t>& dimensions);
        double Add(const TVectorType& x, const TDataType& y);
    };

    /*
        TArgmaxLearner
    */
    class TArgmaxLearner: public TBackPropagationLearner {
    public:
        TArgmaxLearner();
        TArgmaxLearner(double maxInit, const TVector<size_t>& dimensions);
        double Add(const TVectorType& x, const TVectorType& y);
    };

    /*
        TSoftmaxLearner
    */
    class TSoftmaxLearner: public TBackPropagationLearner {
    public:
        TSoftmaxLearner();
        TSoftmaxLearner(double maxInit, const TVector<size_t>& dimensions);
        TSoftmaxLearner(double maxInit, double alpha, const TVector<size_t>& dimensions);
        double Add(const TVectorType& x, size_t target, double score, i32 mask);
        double GradientChecking(const TVectorType& x, size_t target, double score, double eps) const;
    };

}
