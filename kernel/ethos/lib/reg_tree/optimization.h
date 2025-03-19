#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <utility>

#include <functional>

#include <util/thread/pool.h>

namespace NRegTree {

struct TFindFunctionMinimumResult {
    double MinimalFunctionValue;
    double OptimalPoint;

    TFindFunctionMinimumResult(double minimalFunctionValue = 0., double optimalPoint = 0.)
        : MinimalFunctionValue(minimalFunctionValue)
        , OptimalPoint(optimalPoint)
    {
    }

    bool operator < (const TFindFunctionMinimumResult& rhs) const {
        return MinimalFunctionValue < rhs.MinimalFunctionValue;
    }
};

static inline void CalculateValues(const TVector<double>& points,
                                   TVector<double>& values,
                                   std::function<double(double)> func,
                                   IThreadPool& queue,
                                   size_t threadsCount)
{
    Y_ASSERT(points.size() == values.size());

    queue.Start(threadsCount);

    const double* factor = points.begin();
    double* value = values.begin();

    for (; factor != points.end(); ++factor, ++value) {
        queue.SafeAddFunc([=](){
            *value = func(*factor);
        });
    }

    queue.Stop();
}

static inline TFindFunctionMinimumResult FindMinimum(double start,
                                                     double firstStep,
                                                     std::function<double(double)> func,
                                                     size_t threadsCount)
{
    THolder<IThreadPool> queue(CreateThreadPool(threadsCount));

    double left = start;
    double right = start;

    size_t pointsCount = Max(threadsCount, (size_t) 2);

    TVector<double> points(pointsCount);
    TVector<double> values(pointsCount);

    double minimalValue = 0.;

    {
        double preLeft = start;
        double nextStep = 0.;

        for (;;) {
            for (size_t i = 0; i < pointsCount; ++i) {
                points[i] = left + nextStep;
                nextStep = nextStep ? nextStep * 2 : firstStep;
            }
            CalculateValues(points, values, func, *queue, threadsCount);

            size_t minimalLossPosition = MinElement(values.begin(), values.end()) - values.begin();
            minimalValue = values[minimalLossPosition];

            if (minimalLossPosition + 1 == values.size()) {
                preLeft = left + nextStep / 2;
                continue;
            }

            left = minimalLossPosition == 0 ? preLeft : points[minimalLossPosition - 1];
            right = points[minimalLossPosition + 1];
            break;
        }
    }
    if (right < left) {
        std::swap(right, left);
    }

    {
        while (right > left + 1e-3 && fabs(right) > fabs(left) * 1.0001) {
            for (size_t i = 0; i < points.size(); ++i) {
                points[i] = left + (i + 1) * (right - left) / (points.size() + 1);
            }
            CalculateValues(points, values, func, *queue, threadsCount);

            size_t minimalLossPosition = MinElement(values.begin(), values.end()) - values.begin();
            minimalValue = values[minimalLossPosition];

            left = minimalLossPosition == 0 ? left : points[minimalLossPosition - 1];
            right = minimalLossPosition + 1 == values.size() ? right : points[minimalLossPosition + 1];
        }
    }

    return TFindFunctionMinimumResult(minimalValue, (left + right) / 2);
}

static inline TFindFunctionMinimumResult FindMinimum(std::function<double(double)> func, size_t threadsCount, double start = 0.) {
    TFindFunctionMinimumResult onTheRight = FindMinimum(start, +1., func, threadsCount);
    TFindFunctionMinimumResult onTheLeft = FindMinimum(start, -1., func, threadsCount);

    return Min(onTheLeft, onTheRight);
}

}
