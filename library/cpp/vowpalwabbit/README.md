# Vowpal Wabbit Model Aapplier + tools

Vowapl Wabbit model is in fact a big array of feature weights (aka feature table).

The size of this table is 2 ^ b where **b** - is number of bits 
(`-b` or `--bit_precision` vw command line argument)

This library has two purposes:
1. Allows converting models from VowpalWabbit (readable) format to a plain feature table (just an array of float values)
2. Calculate model predictions using a plain format model.

### class NVowpalWabbit::TModel (aka TVowpalWabbitModel)

This class can load model in the plan format or convert VW readable model to the plain format.

***Constructors***

* `TModel(const TString& flatWeightFileName)` - init model from a file
* `TModel(const TBlob& weights)` - init model from a memory blob.

The model file or memory blob must have size equal to 2^b (where b - number of bits)

***Methods***

* `const float* GetWeights() const` - return a pointer to feature table.
* `size_t GetWeightsSize() const` - returns a feature table size (2 ^ b).
* `ui8 GetBits() const` - get number of feature table bits (b).
* `const TBlob& GetBlob() const` - get the blob where feature table is stored.
* `float operator[] (ui32 hash) const` - access featire weight by feature index (or hash). The actual index in the feature table will be `hash % table_size`

### Converting VW readable model

VW model in readable format can be converted to the plain format using 
`NVowpalWabbit::ReadTextModel()` which returns an instance of `NVowpalWabbit::TModel` class.

### class NVowpalWabbit::TPackedModel

This class is similar to `NVowpalWabbit::TModel` but uses 1-byte per feature weight, 
so the model size is 4 times smaller comparing to the plain float format.

### class NVowpalWabbit::THashCalcer

A utility class that simplifies text hash calculations. 

VowpalWabbit uses MurMur hash v3 to obtain hashes for text features. 

***Methods***

`template <typename TStringType>`<br/>`
static void CalcHashes(const TStringBuf ns, const TVector<TStringType>& words, ui32 ngrams, TVector<ui32>& hashes)` - calculates hashes for tokens and their n-grams.

### class TPredictor\<TModel\> : public THashCalcer

An utility class that simplifies VW model prediction over texts using `TModel` or `TPackedModel`
* Calculate hashes for tokens and their n-grams.
* Calculate the resulting model prediction by summing up feature weights for all hashes and the model bias term (method `Predict()`).
