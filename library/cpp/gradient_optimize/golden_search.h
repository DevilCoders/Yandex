#pragma once

#include <functional>

double GoldenSearch(std::function<double(double)>, double a, double b, double err = 1e-10, int maxIter = 30); //TODO: float?
