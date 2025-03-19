#include <kernel/matrixnet/mn_dynamic.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/join.h>

class TMnFeaturesMaker : public NMatrixnet::TMnSseDynamic {
private:
    using TBase = NMatrixnet::TMnSseDynamic;
public:
    TMnFeaturesMaker() {
    }

    TMnFeaturesMaker(const NMatrixnet::TMnSseDynamic& base)
        : TBase(base)
    {
    }

    TVector<float> GetTreeFeatures(const TVector<float>& originalFeatures) const {
        // condition index to factor index
        TVector<size_t> cond2findex;
        // binary features
        TVector<bool> features;
        {
            size_t condIdx = 0;
            for (size_t i = 0; i < this->Matrixnet.Meta.FeaturesSize; ++i) {
                const size_t factorIndex = this->Matrixnet.Meta.Features[i].Index;
                const size_t bordersNum  = this->Matrixnet.Meta.Features[i].Length;
                for (size_t borderIdx = 0; borderIdx < bordersNum; ++borderIdx) {
                    cond2findex.push_back(factorIndex);
                    features.push_back(originalFeatures[factorIndex] > Matrixnet.Meta.Values[condIdx++]);
                }
            }
        }

        TVector<float> treeFeatures;
        {
            size_t dataIdx = this->Matrixnet.Meta.GetSizeToCount(0);
            size_t treeCondIdx = 0;
            const auto& dataHolder = std::get<NMatrixnet::TMultiData>(Matrixnet.Leaves.Data).MultiData.at(0);
            for (size_t condNum = 1; condNum < this->Matrixnet.Meta.SizeToCountSize; ++condNum) {
                for (int treeIdx = 0; treeIdx < this->Matrixnet.Meta.SizeToCount[condNum]; ++treeIdx) {
                    size_t valueIdx = 0;
                    for (size_t i = 0; i < condNum; ++i) {
                        const size_t condIdx = this->Matrixnet.Meta.GetIndex(treeCondIdx++);
                        valueIdx |= features[condIdx] << i;
                    }
                    treeFeatures.push_back(dataHolder.Data[dataIdx + valueIdx] - (1 << 31));
                    treeFeatures.back() *= dataHolder.Norm.DataScale;
                }
            }
        }

        return treeFeatures;
    }
};

typedef TAtomicSharedPtr<const TMnFeaturesMaker> TMnFeaturesMakerPtr;

class TDeepTreeModel {
private:
    TVector<TMnFeaturesMakerPtr> MnFeatureMakers;
public:
    TDeepTreeModel() {
    }

    TDeepTreeModel(const TVector<TString>& modelPaths) {
        for (const TString& modelPath : modelPaths) {
            AddModel(modelPath);
        }
    }

    double Apply(const TVector<float>& originalFeatures) const {
        Y_ASSERT(!MnFeatureMakers.empty());

        double result = 0.;
        TVector<float> features = originalFeatures;
        for (size_t i = 0; i + 1 < MnFeatureMakers.size(); ++i) {
            result += MnFeatureMakers[i]->CalcRelev(features);
            features = MnFeatureMakers[i]->GetTreeFeatures(features);
        }
        result += MnFeatureMakers.back()->CalcRelev(features);

        return result;
    }
private:
    void AddModel(const TString& filename) {
        NMatrixnet::TMnSseDynamic matrixnet;
        {
            TFileInput modelIn(filename);
            matrixnet.Load(&modelIn);
        }
        MnFeatureMakers.push_back(new TMnFeaturesMaker(matrixnet));
    }
};

int ConvertPool(int argc, const char** argv) {
    TString featuresPath;
    TString modelPath;

    {
        NLastGetopt::TOpts opts;
        opts.AddLongOption('f', "features", "features path")
            .Required()
            .StoreResult(&featuresPath);
        opts.AddLongOption('m', "model", "model path")
            .Required()
            .StoreResult(&modelPath);
        NLastGetopt::TOptsParseResult res(&opts, argc,  argv);
    }

    TMnFeaturesMaker featuresMaker;
    {
        TFileInput modelIn(modelPath);
        featuresMaker.Load(&modelIn);
    }

    TString featuresStr;
    TFileInput featuresIn(featuresPath);
    while (featuresIn.ReadLine(featuresStr)) {
        TStringBuf featuresStrBuf(featuresStr);

        const TStringBuf queryId = featuresStrBuf.NextTok('\t');
        const TStringBuf target = featuresStrBuf.NextTok('\t');
        const TStringBuf url = featuresStrBuf.NextTok('\t');
        const TStringBuf weight = featuresStrBuf.NextTok('\t');

        TVector<float> features;
        while (featuresStrBuf) {
            features.push_back(FromString<float>(featuresStrBuf.NextTok('\t')));
        }

        TVector<float> treeFeatures = featuresMaker.GetTreeFeatures(features);

        Cout << queryId << "\t"
             << (FromString<double>(target) - featuresMaker.CalcRelev(features)) << "\t"
             << url << "\t"
             << weight << "\t"
             << JoinSeq("\t", treeFeatures) << "\n";
    }

    return 0;
}

int Apply(int argc, const char** argv) {
    TString featuresPath;
    TVector<TString> modelPaths;

    {
        NLastGetopt::TOpts opts;
        opts.AddLongOption('f', "features", "features path")
            .Required()
            .StoreResult(&featuresPath);
        opts.AddLongOption('m', "models", "model paths, comma-separated")
            .Required()
            .SplitHandler(&modelPaths, ',');
        NLastGetopt::TOptsParseResult res(&opts, argc,  argv);
    }

    TDeepTreeModel deepTreeModel(modelPaths);

    TString featuresStr;
    TFileInput featuresIn(featuresPath);
    while (featuresIn.ReadLine(featuresStr)) {
        TStringBuf featuresStrBuf(featuresStr);

        const TStringBuf queryId = featuresStrBuf.NextTok('\t');
        const TStringBuf target = featuresStrBuf.NextTok('\t');
        const TStringBuf url = featuresStrBuf.NextTok('\t');
        const TStringBuf weight = featuresStrBuf.NextTok('\t');

        TVector<float> features;
        while (featuresStrBuf) {
            features.push_back(FromString<float>(featuresStrBuf.NextTok('\t')));
        }

        Cout << queryId << "\t"
             << target << "\t"
             << url << "\t"
             << weight << "\t"
             << deepTreeModel.Apply(features) << "\n";
    }

    return 0;
}

int main(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode("apply", Apply, "apply deep tree model");
    modChooser.AddMode("convert", ConvertPool, "convert pool");
    return modChooser.Run(argc, argv);
}
