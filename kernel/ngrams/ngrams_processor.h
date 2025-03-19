#pragma once

#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/ysaveload.h>
#include <util/stream/file.h>

#include <library/cpp/on_disk/aho_corasick/reader.h>
#include <library/cpp/on_disk/aho_corasick/writer.h>
#include <kernel/matrixnet/mn_dynamic.h>

class TNgramsProcessor {
public:
    TNgramsProcessor(const char* clustersFile,
                     const char* modelFile);
    TNgramsProcessor(const char* clustersFile);
    ~TNgramsProcessor();

    float Match(const char* url) const;
    size_t LoadClusters(const char* clustersFile);
    void LoadModel(const char* modelFile);
    void CalculateFeatures(const char* url, TVector<float>* featuresVector) const;

    static float Remap(float val) {
        return ClampVal<float>(val * 0.5f + 0.25f, 0.0f, 1.0f);
    }

private:
    NMatrixnet::TMnSseDynamic Model;
    TBlob WordsBlob;
    TDefaultMappedAhoCorasick* AhoWords;
    size_t FeatureCount;
};
