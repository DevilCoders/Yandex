#include "pooldump.h"

#include <util/random/random.h>

namespace NSnippets
{
    const TString poolAlgo[5] = {
        "Algo5_quadro", "Algo4_triple", "Algo3_pairs", "Algo2", "meta_descr"
    };
    const size_t CANDIDATES_NUMBER = 1;

    void TPoolDumpCandidateChooser::Init(const TTops& tops)
    {
        for (TTops::const_iterator curTop = tops.begin(); curTop != tops.end(); ++curTop)
        {
            AlgoCandidateCount[curTop->Name] += curTop->Top.ysize();
        }
        GenerateTasks();
    }

    size_t TPoolDumpCandidateChooser::GetTaskId(const TString& algo, int rank) const
    {
        THashMap<TCandidateId, size_t>::const_iterator it = Tasks.find(std::make_pair(algo, rank));
        if (it != Tasks.end())
            return it->second;
        return 0;
    }

    bool TPoolDumpCandidateChooser::CheckAlgo(TString algoName) const
    {
        THashMap<TString, int>::const_iterator it = AlgoCandidateCount.find(algoName);
        return (it != AlgoCandidateCount.end() && it->second > 0 && RandomNumber<bool>());
    }

    TString TPoolDumpCandidateChooser::GetRandomAlgo() const
    {
        for (size_t algoNum = 0; algoNum < 5; ++algoNum) {
            TString algoName = poolAlgo[algoNum];
            if (CheckAlgo(algoName)) {
                return algoName;
            }
        }
        return poolAlgo[3];
    }

    size_t TPoolDumpCandidateChooser::GetRandomRank(size_t total) const
    {
        return RandomNumber<size_t>(total);
    }

    void TPoolDumpCandidateChooser::GenerateTask(size_t taskNum) {
        TString algoName = GetRandomAlgo();
        THashMap<TString, int>::const_iterator it = AlgoCandidateCount.find(algoName);
        if (it != AlgoCandidateCount.end() && it->second > 0) {
            Tasks[std::make_pair(algoName, GetRandomRank(it->second))] = taskNum;
        }
    }

    void TPoolDumpCandidateChooser::GenerateTasks()
    {
        for (size_t step = 0; step < 100500; ++step) {
            for (size_t taskNum = 1; taskNum <= CANDIDATES_NUMBER; ++taskNum)
                GenerateTask(taskNum);
            if (Tasks.size() == CANDIDATES_NUMBER)
                break;
            Tasks.clear();
        }
    }
}
