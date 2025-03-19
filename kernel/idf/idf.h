#pragma once

#include <util/system/defaults.h>

/*
Here we use the term count instead of the document count in the classic IDF
  termCount - the total number of occurrences of the specific term in the collection
  allTermsCount - the total number of the all terms in the collection (summa of termCount over terms)
  reverseFreq = allTermsCount / termCount
*/

double ReverseFreq2Idf(double reverseFreq);

double TermCount2Idf(i64 allTermsCount, i64 termCount);
i64 TermCount2RevFreq(i64 allTermsCount, i64 termCount);

// this is what becomes 1
const ui32 LINKWEIGHT_BASE = 16;

// normalizing multiplier for 'full' link weight
// note: one bit is already stripped off because there's 16 bits initially,
//       but we put only 15
const float LINKWEIGHT_BASE_FIT = (float)(1. / LINKWEIGHT_BASE);
