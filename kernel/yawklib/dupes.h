#pragma once

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
// isolated checks (mostly for reference purposes), see large-scale versions below

// 1) Checks whether two strings could be just different spellings of the same phrase.
// Understands translit, space remove-inserts, numbers-to-words and vice-versa and some other common transformations
bool IsDuplicates(const TUtf16String &phrase1, const TUtf16String &phrase2);

// 1a) Checks whether first string has a substring "identical" to the second
// as usual, returns TUtf16String::npos if no such substring found, otherwise returns head position
// (like e.g. TUtf16String::find); tail position is not necessary head + substr.size(), so you can obtain it too
size_t FindDuplicate(const TUtf16String &phrase, const TUtf16String &substr, size_t start = 0, size_t *tail = nullptr);

// 2) Calculates Levenshtein distance between two (wide) strings
int LevenshteinDist(const TStringBuf &w1, const TStringBuf &w2);
int LevenshteinDist(const TWtringBuf &w1, const TWtringBuf &w2);

// 3) checks whether the two phrases are one of the form [phrase1 phrase2], and the second - [phrase2 phrase1]
bool IsReorderDuplicates(const TUtf16String &phrase1, const TUtf16String &phrase2);

////////////////////////////////////////////////////////////////////////////////////////////////////
typedef TVector<std::pair<int, int> > TDupes;

// Large-scale versions
// Do not compare every string to every other, so it is ok to use it when src.length() is about 1 mln

// Selects pairs that are very close in the sense of levenshtein distance (misspellings, orpho-variants etc)
void SelectLevenshteinDupes(const TVector<TUtf16String> &src, TDupes *res);
void SelectLevenshteinDupesForLongPhrases(const TVector<TUtf16String> &src, float maxDistanceRateForDuplicate, int shingleSize, TDupes *res);

// Selects pairs that are duplicates in the sense of IsDuplicates
// dupesVariants is an optional parameter that describes what substrings are considered indistinguishable spelling variants
// An example is TheDupesVariants array that can be found in the corresponding .cpp file
void SelectDuplicates(const TVector<TUtf16String> &src, TDupes *res, const char **dupesVariants = nullptr, bool printProfilingCounter = false);

// Selects pairs that are duplicates in the sense of a wrong keyboard scheme
void SelectRuEnKeybDuplicates(const TVector<TUtf16String> &src, TDupes *res);

// Selects pairs that are duplicates in the sense of IsReorderDuplicates
void SelectReorderDupes(const TVector<TUtf16String> &src, TDupes *res);
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef TVector<TVector<int> > TClusters;
void PairsToClusters(const TDupes &src, TClusters *resClusters);
void AddSingleElementClusters(size_t numElements, TClusters *clusters);
