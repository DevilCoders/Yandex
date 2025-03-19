#pragma once

//
// Created by Mikhail Yutman on 30.03.2020.
//

#include "distance.h"
#include "naive_dp.h"
#include "restore.h"
#include "structures.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/array_ref.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/split.h>

#include <algorithm>
#include <cstdlib>

template <class T>
TRecordJoin JoinMarkups(const IDistance<T>& distance, TUnitedRecordBitsMarkups& bitsMarkups) {
    /*Cerr << bitsMarkups.BitsMarkups.size() << Endl;
    for (auto& markup : bitsMarkups.BitsMarkups) {
        Cerr << markup << Endl;
    }*/

    if (bitsMarkups.BitsMarkups.empty()) {
        return TRecordJoin(bitsMarkups.RecordId);
    }

    if (bitsMarkups.BitsMarkups.size() == 1) {
        return TRecordJoin(bitsMarkups.RecordId, bitsMarkups.BitsMarkups[0]);
    }

    /*if (bitsMarkups.RecordId == "031109b1-d4b3-4c2e-a4c8-a4d566bbd4fe") {
        Cerr << "kek" << Endl;
    }*/

    /**
     * Calculating the minimal value of distance metric for word sequence
     */

    TVector<TDpBestStates> pr;

    ui32 rightLast;
    ui32 rightPrevious;
    ui32 leftLast;

    std::tie(rightLast, rightPrevious, leftLast) = CalculateDp<T>(distance, bitsMarkups, &pr);

    /**
     * Restoring the minimal word sequence from written pr values
     */

    return RestoreMarkups(
        distance,
        bitsMarkups,
        pr,
        rightLast,
        rightPrevious,
        leftLast);
}

TVector<TRecordBitsMarkups> ReadRecordsBitsMarkupsFromJson(TStringBuf filename, int offset);

/**
 * Read records bits markups from input file of join_markups input format
 * @param filename input filename
 * @param offset records split offset in milliseconds
 * @return vector of record bits markups
 */
TVector<TRecordBitsMarkups> ReadRecordsBitsMarkupsFromTxt(
    TStringBuf filename,
    int offset);

/**
 * Unite markups referring to same bits
 * @param recordBitsMarkups vector containing vectors of all markups referring to corresponding record bit for all bits of current record
 * @return united markups for record bits
 */
template <class T>
TUnitedRecordBitsMarkups UniteBitsMarkups(const IDistance<T>& distance, const TRecordBitsMarkups& recordBitsMarkups) {
    TUnitedRecordBitsMarkups unitedRecordsBitsMarkups(recordBitsMarkups.RecordId, recordBitsMarkups.S3Key);
    for (auto& bitMarkups : recordBitsMarkups.BitsMarkups) {
        unitedRecordsBitsMarkups.BitsMarkups.push_back(UniteBitMarkups<T>(distance, bitMarkups));
    }
    return unitedRecordsBitsMarkups;
}

/**
 * Joins records bits markups ui32o records markups
 * @param recordsBitsMarkups vector of record bits markups
 * @return vector of record joins
 */
template <class T>
TVector<TRecordJoin> JoinRecordsBitsMarkups(const IDistance<T>& distance, TConstArrayRef<TRecordBitsMarkups> recordsBitsMarkups) {
    TVector<TRecordJoin> recordsJoins;
    for (auto& recordBitsMarkups : recordsBitsMarkups) {
        auto unitedBitsMarkups = UniteBitsMarkups<T>(distance, recordBitsMarkups);
        for (auto markup : unitedBitsMarkups.BitsMarkups) {
            Cerr << markup.JoinWithWhitespaces() << Endl;
        }
        auto recordJoin = JoinMarkups<T>(distance, unitedBitsMarkups);
        recordsJoins.push_back(recordJoin);
    }
    return recordsJoins;
}

/**
 * Writes records markups to stdout in join_markups output format
 * @param recordsMarkups vector of record joins
 */
void OutputRecordsMarkupsToOutputStream(TConstArrayRef<TRecordJoin> recordsMarkups, IOutputStream& stream);

void OutputRecordsMarkupsAsJson(TConstArrayRef<TRecordJoin> recordsMarkups, IOutputStream& stream, const TString& tag, const TString& idTag);
