#pragma once

#include <util/generic/vector.h>

template <class TPtr>
class TCombinationBuilder
{
private:
    TVector< TVector<TPtr> >    CombinationItems;
    TVector< size_t >           CombinationIndex;
    TVector< TPtr >             Combination;

public:
    TCombinationBuilder(TVector< TVector<TPtr> > items)
        : CombinationItems(items)
    {
        for(size_t i = 0; i < CombinationItems.size(); i++) {
            CombinationIndex.push_back(0);
            Combination.push_back(TPtr());
        }

        BuildCombination();
    }

    bool Shift()
    {
        size_t shiftIndex = 0;

        do {
            if (shiftIndex == CombinationItems.size()) {
                return false;
            }
            if (CombinationItems[shiftIndex].size() == 0){
                shiftIndex++;
                continue;
            }
            if (CombinationItems[shiftIndex].size() == CombinationIndex[shiftIndex] + 1){
                CombinationIndex[shiftIndex] = 0;
                shiftIndex++;
                continue;
            }
            CombinationIndex[shiftIndex]++;
            break;
        } while(true);

        BuildCombination();

        return true;
    }

    const TVector< TPtr >& GetCombination() const {
        return Combination;
    }

private:
    void BuildCombination() {
        for(size_t i = 0; i < CombinationItems.size(); i++) {
            if (CombinationItems[i].size() > 0) {
                Combination[i] = CombinationItems[i][CombinationIndex[i]];
            }
        }
    }

};

