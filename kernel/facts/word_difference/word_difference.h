#pragma once
#include <util/charset/wide.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
void CalcLemmaDifferences(TUtf16String q1, TUtf16String q2, TUtf16String &res1, TUtf16String &res2);
void CalcExactWordDifferences(TUtf16String q1, TUtf16String q2, TUtf16String &res1, TUtf16String &res2);

void CalcLemmaDifferences(const TVector<TUtf16String> &qWords1, const TVector<TUtf16String> &qWords2, TUtf16String &res1, TUtf16String &res2);
void CalcExactWordDifferences(const TVector<TUtf16String> &qWords1, const TVector<TUtf16String> &qWords2, TUtf16String &res1, TUtf16String &res2);
