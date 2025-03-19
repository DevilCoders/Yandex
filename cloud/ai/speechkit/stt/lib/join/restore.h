#pragma once

//
// Created by Mikhail Yutman on 29.04.2020.
//

#pragma once

#include "distance.h"
#include "structures.h"

#include <util/generic/array_ref.h>
#include <util/generic/fwd.h>

#include <cstddef>

void AddReversedToAns(const TText& added, TText& ans);

template <class T>
void RestoreLastLayerAnswer(
    const IDistance<T>& distance, const TText& lastBit, const TText& preLastBit, ui32 rightLast, ui32 leftLast,
    ui32 rightPreLast, TText& ans) {
    auto lastSuffix = lastBit.SubText(rightLast);

    AddReversedToAns(
        lastSuffix,
        ans);

    for (double x : distance.RestoreLikelihoods(lastSuffix, lastSuffix)) {
        Cout << x << ", ";
    }

    auto preLastBitChunk = preLastBit.SubText(rightPreLast);
    auto lastBitChunk = lastBit.SubText(leftLast, rightLast);

    auto united = UniteBitMarkups<T>(
        distance,
        {preLastBitChunk,
         lastBitChunk});

    for (auto& chunk : {preLastBitChunk, lastBitChunk}) {
        for (double x : distance.RestoreLikelihoods(chunk, united)) {
            Cout << x << ", ";
        }
    }

    AddReversedToAns(
        united,
        ans);
}

template <class T>
void RestoreMiddleLayerAnswer(
    const IDistance<T>& distance,
    const TText& currentBit,
    const TText& previousBit,
    const TText& prePreviousBit,
    ui32 leftCurrent,
    ui32 rightPrevious,
    ui32 rightPrePrevious,
    ui32 leftPrevious,
    TText& ans) {
    auto currentBitChunk = currentBit.SubText(0, leftCurrent);
    auto previousBitChunk = previousBit.SubText(leftPrevious, rightPrevious);
    auto prePreviousBitChunk = prePreviousBit.SubText(rightPrePrevious);
    auto united = UniteBitMarkups<T>(
        distance,
        {currentBitChunk,
         previousBitChunk,
         prePreviousBitChunk});

    for (auto& chunk : {currentBitChunk, previousBitChunk, prePreviousBitChunk}) {
        for (double x : distance.RestoreLikelihoods(chunk, united)) {
            Cout << x << ", ";
        }
    }

    AddReversedToAns(
        united,
        ans);
}

template <class T>
void RestoreFirstLayerAnswer(
    const IDistance<T>& distance,
    const TText& zeroBit,
    const TText& firstBit,
    ui32 leftFirst,
    ui32 rightZero,
    ui32 leftZero,
    TText& ans) {
    auto firstBitChunk = firstBit.SubText(0, leftFirst);
    auto zeroBitChunk = zeroBit.SubText(leftZero, rightZero);

    auto united = UniteBitMarkups<T>(
        distance,
        {firstBitChunk,
         zeroBitChunk});

    for (auto& chunk : {firstBitChunk, zeroBitChunk}) {
        for (double x : distance.RestoreLikelihoods(chunk, united)) {
            Cout << x << ", ";
        }
    }

    AddReversedToAns(
        united,
        ans);

    auto zeroBitPrefix = zeroBit.SubText(0, leftZero);
    for (double x : distance.RestoreLikelihoods(zeroBitPrefix, zeroBitPrefix)) {
        Cout << x << ", ";
    }

    AddReversedToAns(
        zeroBitPrefix,
        ans);
}

template <class T>
TRecordJoin RestoreMarkups(
    const IDistance<T>& distance,
    const TUnitedRecordBitsMarkups& bitsMarkups,
    TConstArrayRef<TDpBestStates> pr,
    ui32 rightLast,
    ui32 rightPrevious,
    ui32 leftLast) {
    TRecordJoin recordJoin(bitsMarkups.RecordId, bitsMarkups.S3Key);

    RestoreLastLayerAnswer<T>(
        distance,
        bitsMarkups.BitsMarkups.back(),
        bitsMarkups.BitsMarkups[bitsMarkups.BitsMarkups.size() - 2],
        rightLast,
        leftLast,
        rightPrevious,
        recordJoin.JoinedText);

    for (int currentWordIndex = bitsMarkups.BitsMarkups.ysize() - 1; currentWordIndex > 1; currentWordIndex--) {
        ui32 rightPrePrevious = pr[currentWordIndex - 1].States[rightPrevious][leftLast].PreviousLevelR;
        ui32 leftPrevious = pr[currentWordIndex - 1].States[rightPrevious][leftLast].CurrentLevelL;

        Cerr << "Level " << (currentWordIndex - 1) << Endl;
        for (ui32 i = 0; i < pr[currentWordIndex - 1].States.size(); i++) {
            for (ui32 j = 0; j < pr[currentWordIndex - 1].States[i].size(); j++) {
                Cerr << "(" << pr[currentWordIndex - 1].States[i][j].CurrentLevelL << ", " << pr[currentWordIndex - 1].States[i][j].PreviousLevelR << ") ";
            }
            Cerr << Endl;
        }

        RestoreMiddleLayerAnswer<T>(
            distance,
            bitsMarkups.BitsMarkups[currentWordIndex],
            bitsMarkups.BitsMarkups[currentWordIndex - 1],
            bitsMarkups.BitsMarkups[currentWordIndex - 2],
            leftLast,
            rightPrevious,
            rightPrePrevious,
            leftPrevious,
            recordJoin.JoinedText);

        rightPrevious = rightPrePrevious;
        leftLast = leftPrevious;
    }

    ui32 leftZero = pr[0].States[rightPrevious][leftLast].CurrentLevelL;

    RestoreFirstLayerAnswer<T>(
        distance,
        bitsMarkups.BitsMarkups[0],
        bitsMarkups.BitsMarkups[1],
        leftLast,
        rightPrevious,
        leftZero,
        recordJoin.JoinedText);

    recordJoin.JoinedText.Reverse();
    return recordJoin;
}
