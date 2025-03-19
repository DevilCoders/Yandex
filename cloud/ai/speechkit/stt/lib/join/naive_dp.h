#pragma once

//
// Created by Mikhail Yutman on 29.04.2020.
//

#pragma once

#include "distance.h"
#include "structures.h"

template <class T>
void CalculateStartDpLayer(
    const IDistance<T>& distance,
    const TText& zeroBit,
    const TText& firstBit,
    TDpValues<T>& startDpLayer,
    TDpBestStates& startPrLayer) {
    ui32 leftZeroL;
    ui32 leftZeroR;
    std::tie(leftZeroL, leftZeroR) = zeroBit.Bounds(0);
    //Cerr << "left zero: " << leftZeroL << " " << leftZeroR << Endl;

    ui32 rightZeroL;
    ui32 rightZeroR;
    std::tie(rightZeroL, rightZeroR) = zeroBit.Bounds(1);
    //Cerr << "right zero: " << rightZeroL << " " << rightZeroR << Endl;

    ui32 leftFirstL;
    ui32 leftFirstR;
    std::tie(leftFirstL, leftFirstR) = firstBit.Bounds(0);
    //Cerr << "left first: " << leftFirstL << " " << leftFirstR << Endl;

    for (int rightZero = 0; rightZero <= zeroBit.Size(); rightZero++) {
        for (int leftFirst = 0; leftFirst <= firstBit.Size(); leftFirst++) {
            startDpLayer.Values[rightZero][leftFirst] = distance.Max();
        }
    }

    for (ui32 rightZero = rightZeroL; rightZero <= rightZeroR; rightZero++) {
        for (ui32 leftFirst = leftFirstL; leftFirst <= leftFirstR; leftFirst++) {
            for (ui32 leftZero = leftZeroL; leftZero <= std::min(leftZeroR, rightZero); leftZero++) {
                auto zeroBitChunk = zeroBit.SubText(leftZero, rightZero);
                auto firstBitChunk = firstBit.SubText(0, leftFirst);
                T val = distance.CalculateDistance(zeroBitChunk, firstBitChunk); //+ (ui32) zeroBitAppendixChunk.size();
                if (val < (T)startDpLayer.Values[rightZero][leftFirst]) {
                    startDpLayer.Values[rightZero][leftFirst] = val;
                    startPrLayer.States[rightZero][leftFirst] = TState(0, leftZero);
                }
            }
        }
    }
}

template <class T>
void CalculateMiddleDpLayer(
    const IDistance<T>& distance,
    const TDpValues<T>& previousDpLayer,
    const TText& previousBit,
    const TText& prePreviousBit,
    const TText& currentBit,
    TDpBestStates& currentPrLayer,
    TDpValues<T>& currentDpLayer) {
    ui32 rightPrePreviousL;
    ui32 rightPrePreviousR;
    std::tie(rightPrePreviousL, rightPrePreviousR) = prePreviousBit.Bounds(1);
    //Cerr << "rightPrePrevious: " << rightPrePreviousL << " " << rightPrePreviousR << Endl;

    ui32 leftPreviousL;
    ui32 leftPreviousR;
    std::tie(leftPreviousL, leftPreviousR) = previousBit.Bounds(0);
    //Cerr << "leftPrevious: " << leftPreviousL << " " << leftPreviousR << Endl;

    ui32 rightPreviousL;
    ui32 rightPreviousR;
    std::tie(rightPreviousL, rightPreviousR) = previousBit.Bounds(1);
    //Cerr << "rightPrevious: " << rightPreviousL << " " << rightPreviousR << Endl;

    ui32 leftCurrentL;
    ui32 leftCurrentR;
    std::tie(leftCurrentL, leftCurrentR) = currentBit.Bounds(0);
    //Cerr << "leftCurrent: " << leftCurrentL << " " << leftCurrentR << Endl;

    for (int rightPrevious = 0; rightPrevious <= previousBit.Size(); rightPrevious++) {
        for (int leftCurrent = 0; leftCurrent <= currentBit.Size(); leftCurrent++) {
            currentDpLayer.Values[rightPrevious][leftCurrent] = distance.Max();
        }
    }

    for (ui32 leftCurrent = leftCurrentL; leftCurrent <= leftCurrentR; leftCurrent++) {
        auto currentBitChunk = currentBit.SubText(0, leftCurrent);
        auto dists31 = distance.CalculateAllDistancesBackwardTemp(currentBitChunk, prePreviousBit);
        for (ui32 rightPrevious = rightPreviousL; rightPrevious <= rightPreviousR; rightPrevious++) {
            auto previousBitPrefix = previousBit.SubText(0, rightPrevious);
            auto dists21 = distance.CalculateAllDistancesBackward(previousBitPrefix, prePreviousBit);
            auto dists32 = distance.CalculateAllDistancesBackwardTemp(currentBitChunk, previousBitPrefix);
            for (int leftPrevious = std::min(rightPrevious, leftPreviousR); leftPrevious >= (int)leftPreviousL; leftPrevious--) {
                for (int rightPrePrevious = rightPrePreviousR; rightPrePrevious >= (int)rightPrePreviousL; rightPrePrevious--) {
                    T dist12 = dists21[leftPrevious][rightPrePrevious];
                    T dist13 = dists31[rightPrePrevious];
                    T dist23 = dists32[leftPrevious];
                    T val = (T)previousDpLayer.Values[rightPrePrevious][leftPrevious] + dist23 + dist12 + dist13;
                    //+ std::min(dist12 + dist13, std::min(dist12 + dist23, dist13 + dist23));
                    if ((T)currentDpLayer.Values[rightPrevious][leftCurrent] > val) {
                        currentDpLayer.Values[rightPrevious][leftCurrent] = val;
                        currentPrLayer.States[rightPrevious][leftCurrent] = TState(rightPrePrevious, leftPrevious);
                    }
                }
            }
        }
    }
}

template <class T>
std::tuple<ui32, ui32, ui32> CalculateLastDpLayer(
    const IDistance<T>& distance,
    const TDpValues<T>& previousDpLayer,
    const TText& lastBit,
    const TText& preLastBit) {
    //typedef ui32 T;
    ui32 rightPreLastL;
    ui32 rightPreLastR;
    std::tie(rightPreLastL, rightPreLastR) = preLastBit.Bounds(1);
    //Cerr << "rightPreLast: " << rightPreLastL << " " << rightPreLastR << Endl;

    ui32 leftLastL;
    ui32 leftLastR;
    std::tie(leftLastL, leftLastR) = lastBit.Bounds(0);
    //Cerr << "leftLast: " << leftLastL << " " << leftLastR << Endl;

    ui32 rightLastL;
    ui32 rightLastR;
    std::tie(rightLastL, rightLastR) = lastBit.Bounds(1);
    //Cerr << "rightLast: " << rightLastL << " " << rightLastR << Endl;

    T mn = distance.Max();
    std::tuple<ui32, ui32, ui32> best = {0, 0, 0};
    for (ui32 rightPreLast = rightPreLastL; rightPreLast <= rightPreLastR; rightPreLast++) {
        for (ui32 leftLast = leftLastL; leftLast <= leftLastR; leftLast++) {
            for (ui32 rightLast = std::max(leftLast, rightLastL); rightLast <= rightLastR; rightLast++) {
                auto preLastBitChunk = preLastBit.SubText(rightPreLast);
                auto lastBitChunk = lastBit.SubText(leftLast, rightLast);
                auto lastBitAppendixChunk = lastBit.JoinWithWhitespaces(rightLast);
                T dist = distance.CalculateDistance(preLastBitChunk, lastBitChunk);
                T val = (T)previousDpLayer.Values[rightPreLast][leftLast] + dist;
                if (val < mn) {
                    mn = val;
                    best = {rightLast, rightPreLast, leftLast};
                }
            }
        }
    }

    Cerr << "Min dp value: " << mn << Endl;

    /*TLikelihoodValue val = mn;
    if (val.Corresponding > 0) {
        double ans = (val.Likelihood + val.Corresponding * ADDITIONAL) * (SIGMA * SIGMA) / val.Corresponding;
        //Cerr << val.Likelihood << " " << val.Corresponding * ADDITIONAL << Endl;
        Cerr << "(" << ans << ", " << val.Corresponding << "), ";
    }*/

    return best;
}

template <class T>
std::tuple<ui32, ui32, ui32> CalculateDp(
    const IDistance<T>& distance,
    const TUnitedRecordBitsMarkups& bitsMarkups,
    TVector<TDpBestStates>* pr_) {
    int maxWordCount = 1;
    for (const auto& markup : bitsMarkups.BitsMarkups) {
        maxWordCount = Max(maxWordCount, markup.Size() + 1);
    }

    TVector<TDpValues<T>> dp(
        bitsMarkups.BitsMarkups.size() - 1,
        TDpValues<T>(maxWordCount, maxWordCount));

    TVector<TDpBestStates> pr(
        bitsMarkups.BitsMarkups.size() - 1,
        TDpBestStates(
            maxWordCount,
            maxWordCount));

    CalculateStartDpLayer<T>(distance, bitsMarkups.BitsMarkups[0], bitsMarkups.BitsMarkups[1], dp[0], pr[0]);

    for (int currentWordIndex = 2; currentWordIndex < bitsMarkups.BitsMarkups.ysize(); currentWordIndex++) {
        /*Cerr << bitsMarkups.BitsMarkups[currentWordIndex - 1].Words.size() + 1 << " "
             << bitsMarkups.BitsMarkups[currentWordIndex].Words.size() + 1 << Endl;*/
        CalculateMiddleDpLayer<T>(
            distance,
            dp[currentWordIndex - 2],
            bitsMarkups.BitsMarkups[currentWordIndex - 1],
            bitsMarkups.BitsMarkups[currentWordIndex - 2],
            bitsMarkups.BitsMarkups[currentWordIndex],
            pr[currentWordIndex - 1],
            dp[currentWordIndex - 1]);
    }

    *pr_ = pr;

    return CalculateLastDpLayer<T>(
        distance,
        dp.back(),
        bitsMarkups.BitsMarkups.back(),
        bitsMarkups.BitsMarkups[bitsMarkups.BitsMarkups.size() - 2]);
}
