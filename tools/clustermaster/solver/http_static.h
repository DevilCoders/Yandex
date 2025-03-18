#pragma once

#include <tools/clustermaster/communism/util/static_reader.h>

#include <util/generic/singleton.h>

class TSolverStaticReader: public TStaticReader {
public:
    TSolverStaticReader();
};

inline const TSolverStaticReader& GetSolverStaticReader() {
    return *Singleton<TSolverStaticReader>();
}
