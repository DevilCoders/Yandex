#include "readable_repr.h"

#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/generic/vector.h>

namespace NVowpalWabbit {
    TModelAsHash LoadReadableModel(const TString& modelFile) {
        TModelAsHash model;
        TFileInput inp(modelFile);
        TString line;
        TString feature;
        float weight;
        while (inp.ReadLine(line)) {
            Split(line, ':', feature, weight);
            model[feature] = weight;
        }
        return model;
    }

    float ApplyReadableModel(const TModelAsHash& model, const TVector<TString>& sample, size_t order) {
        float wt = *model.FindPtr("Constant");
        for (size_t i = 0; i < sample.size(); ++i) {
            TString curNgram;
            for (size_t j = 0; j < Min(order, sample.size() - i); ++j) {
                if (j)
                    curNgram += ' ';
                curNgram += sample[i + j];
                const auto* wtPtr = model.FindPtr(curNgram);
                wt += wtPtr ? *wtPtr : 0.;
            }
        }
        return wt;
    }

}
