#include "batch_gsk.h"

const int MAX_DFA_POS = 220;

struct TBatchDFAState
{
    union {
        struct {
            ui8 ActivePos, State;
        };
        ui16 PosState;
    };
};

static void ApplyList(TBatchDFAState *statesPtr, int pos, int len,
                      const TBatchRegexpCalcer::TStateFlipInfo *flip, const TBatchRegexpCalcer::TStateFlipInfo *flipEnd)
{
    ui8 poslen = pos + len;
    for (; flip < flipEnd; ++flip) {
        TBatchDFAState &s = statesPtr[flip->RegexpId];
        //if (s.State == flip->SrcState && s.ActivePos <= pos) {
        unsigned sPosState = (unsigned)s.PosState;
        if ((sPosState - (unsigned)flip->SrcStateHack) <= (unsigned)pos) {
            s.ActivePos = poslen;//pos + len;
            ++s.State;
        }
    }
}

double ComputeModel(const TBatchRegexpCalcer &data, const TVector<int> &str, const float *fFactors)
{
    int dfaCount = data.AcceptStates.ysize();
    TVector<TBatchDFAState> states;
    states.resize(dfaCount);
    memset(&states[0], 0, sizeof(states[0])*dfaCount);
    const TBatchRegexpCalcer::TStateFlipInfo *flipPtr =&data.AllStateFlips[0];
    int strLen = str.ysize();
    for (int pos = 0, dfaPos = 0; pos < strLen; ++pos, ++dfaPos) {
        if (dfaPos == MAX_DFA_POS) {
            dfaPos -= MAX_DFA_POS;
            for (int i = 0; i < dfaCount; ++i) {
                TBatchDFAState &s = states[i];
                if (s.ActivePos < MAX_DFA_POS)
                    s.ActivePos = 0;
                else
                    s.ActivePos -= MAX_DFA_POS;
            }

        }
        TSimpleHashTrie::TIterator z(data.Words);
        for (int len = 1; len <= str.ysize() - pos; ++len) {
            if (!z.NextChar(str[pos + len - 1]))
                break;
            int wordId = z.GetWordId();
            if (wordId < 0)
                continue;
            int start = data.WordTransitions[wordId * 4 + TBatchRegexpCalcer::NEXT_PTR];
            int fin = start;
            if (pos == 0) {
                Y_ASSERT(TBatchRegexpCalcer::START_NEXT_PTR == TBatchRegexpCalcer::NEXT_PTR + 1);
                fin = data.WordTransitions[wordId * 4 + TBatchRegexpCalcer::NEXT_PTR + 2];
            } else {
                fin = data.WordTransitions[wordId * 4 + TBatchRegexpCalcer::NEXT_PTR + 1];
            }
            ApplyList(&states[0], dfaPos, len, flipPtr + start, flipPtr + fin);

            if (pos + len == strLen) {
                start = data.WordTransitions[wordId * 4 + TBatchRegexpCalcer::FINISH_NEXT_PTR];
                if (pos == 0) {
                    Y_ASSERT(TBatchRegexpCalcer::START_FINISH_NEXT_PTR == TBatchRegexpCalcer::FINISH_NEXT_PTR + 1);
                    fin = data.WordTransitions[wordId * 4 + TBatchRegexpCalcer::FINISH_NEXT_PTR + 2];
                } else {
                    fin = data.WordTransitions[wordId * 4 + TBatchRegexpCalcer::FINISH_NEXT_PTR + 1];
                }
                ApplyList(&states[0], dfaPos, len, flipPtr + start, flipPtr + fin);
            }
        }
    }
    i64 res = 0;
    for (TVector<TBatchRegexpCalcer::TTree>::const_iterator t = data.Model.begin(); t != data.Model.end(); ++t) {
        const TBatchRegexpCalcer::TTree &info = *t;
        int idx = 0;
        int conditionCount = info.FeatureIds.ysize();
        for (int conditionId = 0; conditionId < conditionCount; ++conditionId) {
            idx <<= 1;
            int f = info.FeatureIds[conditionId];
            if (f & TBatchRegexpCalcer::BINARY_FEATURE) {
                const TBatchRegexpCalcer::TBinaryFeatureStat &bf = data.BinaryFeatures[f ^ TBatchRegexpCalcer::BINARY_FEATURE];
                idx |= (fFactors[bf.SrcIdx] > bf.Border) ? 1 : 0;
            } else {
                int regexpId = f;
                idx |= (states[regexpId].State == data.AcceptStates[regexpId]) ? 1 : 0;
            }
        }
        res += info.BinValues[idx];
    }
    return res * data.ResultScale;
}
