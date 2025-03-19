#include "make_clusters.h"

#include <contrib/libs/opencv/include/opencv2/opencv.hpp>


namespace {
    TVector<TVector<float>> CalcDistances(const cv::Mat& first, const cv::Mat& second) {
        TVector<TVector<float>> distances(first.rows);
        for (int i = 0; i < first.rows; ++i) {
            distances[i].resize(second.rows);
            for (int j = 0; j < second.rows; ++j) {
                distances[i][j] = cv::norm(first.row(i), second.row(j));
            }
        }
        return distances;
    }
}

namespace NClustering {
    TResult KMeans(const TVector<TEmbed>& embeddings,
        const NProto::TClusteringParams& clusteringParams)
    {
        TResult clusters;
        if (embeddings.empty()) {
            return clusters;
        }

        cv::Mat mat(embeddings.size(), embeddings.front().size(), cv::DataType<float>::type);
        for (int i = 0; i < mat.rows; ++i) {
            for (int j = 0; j < mat.cols; ++j) {
                mat.at<float>(i, j) = embeddings[i][j];
            }
        }
        clusters.Labels.resize(embeddings.size());

        if (embeddings.size() <= clusteringParams.GetNumClusters()) {
            for (size_t i = 0; i < embeddings.size(); ++i) {
                clusters.Labels[i] = i;
            }
            clusters.Distances = CalcDistances(mat, mat);
            clusters.Centroids = embeddings;
        } else {
            cv::Mat cvLabels, cvCenters;
            cv::TermCriteria criteria(
                CV_TERMCRIT_ITER | CV_TERMCRIT_EPS,
                clusteringParams.GetKMeansParams().GetMaxIterations(),
                clusteringParams.GetKMeansParams().GetEpsilon()
            );
            cv::kmeans(
                mat, clusteringParams.GetNumClusters(), cvLabels, criteria,
                clusteringParams.GetKMeansParams().GetNumAttempts(), cv::KMEANS_PP_CENTERS, cvCenters
            );

            for (size_t i = 0; i < embeddings.size(); ++i) {
                clusters.Labels[i] = cvLabels.at<int>(i, 0);
            }
            clusters.Distances = CalcDistances(cvCenters, mat);

            clusters.Centroids.resize(cvCenters.rows);
            for(int i = 0; i < cvCenters.rows; ++i) {
                clusters.Centroids[i].resize(cvCenters.cols);
                for(int j = 0; j < cvCenters.cols; ++j) {
                    clusters.Centroids[i][j] = cvCenters.at<float>(i, j);
                }
            }
        }
        return clusters;
    }

    TResult MakeClusters(const TVector<TEmbed>& embeddings,
        const NProto::TClusteringParams& clusteringParams)
    {
        switch (clusteringParams.GetTypeSpecificParamsCase()) {
            case NProto::TClusteringParams::kKMeansParams:
                return KMeans(embeddings, clusteringParams);

            default:
                ythrow yexception() << "Unknown clustering type" << Endl;
        }
    }
}
