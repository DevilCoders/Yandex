#pragma once

#include "dump.h"

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/map.h>

namespace NSnippets
{
    class TPoolDumpCandidateChooser
    {
    private:
        THashMap<TString, int> AlgoCandidateCount;
        typedef std::pair<TString, size_t> TCandidateId;
        THashMap<TCandidateId, size_t> Tasks;
        bool CheckAlgo(TString algoName) const;
        TString GetRandomAlgo() const;
        size_t GetRandomRank(size_t total) const;
        void GenerateTask(size_t taskNum);
        void GenerateTasks();

    public:
        TPoolDumpCandidateChooser() {};
        void Init(const TTops& tops);
        size_t GetTaskId(const TString& algo, int rank) const;
    };

    extern const TString poolAlgo[5];
}
