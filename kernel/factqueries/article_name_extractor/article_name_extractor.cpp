#include "article_name_extractor.h"

#include <kernel/normalize_by_lemmas/normalize.h>

#include <util/charset/wide.h>
#include <util/charset/unidata.h>
#include <util/string/strip.h>

// The order of elements in this vector is important. If a delimiter B is a substring of a delimiter A,
// A should go first. Otherwise, the "difference" will become attached to articleName or answerBody.

static const TVector<TUtf16String> DEFINITION_DELIMITERS = {
    u"-- это",
    u"--это",
    u"‒это",
    u"–это",
    u"—это",
    u"―это",
    u"-это",
    u"‒ это",
    u"– это",
    u"— это",
    u"― это",
    u"- это",
    u"это",
    u"‒",
    u"–",
    u"—",
    u"―",
    u"-"
};

static const wchar16 STRESS_MARK = L'\u0301';
// As opposed to "__when__", "__how_long__" etc.
static const TString MARKER_MARKER = "__marker__";
static const TString DOUBLE_UNDERSCORE = "__";
static const size_t MAX_LETTERS_IN_HEADLINE = 35;
static const size_t MAX_WORDS_IN_HEADLINE = 5;

static TString NormalizeByLemmasUTF8(const TStringBuf& query, const TNormalizeByLemmasInfo& normalizeInfo) {
    return WideToUTF8(NormalizeByLemmas(UTF8ToWide(query), normalizeInfo));
}

static TString NormalizeByLemmasUTF8(const TUtf16String& query, const TNormalizeByLemmasInfo& normalizeInfo) {
    return WideToUTF8(NormalizeByLemmas(query, normalizeInfo));
}

static TUtf16String PrettifyArticleName(const TUtf16String& wtext) {
    TUtf16String result;
    int balance = 0;
    bool inSpaceSequence = false;

    for (auto it = wtext.begin(); it != wtext.end(); ++it) {
        const wchar16 wch = *it;

        if (wch == '(') {
            ++balance;
        } else if (wch == ')') {
            --balance;
        } else if (balance == 0 && wch != STRESS_MARK && IsPrint(wch)) {
            if (IsSpace(wch) || IsBlank(wch)) {
                if (!inSpaceSequence) {
                    result.append(' ');
                }
                inSpaceSequence = true;
            } else {
                result.append(wch);
                inSpaceSequence = false;
            }
        }
    }

    return StripString(result);
}

// After this particular normalization
static bool OnlyMarkerMarker(const TString& query, const THolder<TNormalizeByLemmasInfo>& normalizeInfo) {
    TString normQuery = NormalizeByLemmasUTF8(query, *(normalizeInfo.Get()));

    return (normQuery.EndsWith(MARKER_MARKER) &&
        !StripString(
            normQuery.substr(0, normQuery.size() - MARKER_MARKER.size())
        ).EndsWith(DOUBLE_UNDERSCORE));
}

// After any of given normalizations
static bool OnlyMarkerMarker(const TString& query, const TVector<THolder<TNormalizeByLemmasInfo>>& normalizeInfos) {
    for (const THolder<TNormalizeByLemmasInfo>& normalizeInfo: normalizeInfos) {
        if (OnlyMarkerMarker(query, normalizeInfo)) {
            return true;
        }
    }

    return false;
}

bool TryMakeArticleName(const TString& query,
                        const TString& text,
                        const TVector<THolder<TNormalizeByLemmasInfo>>& normalizeInfos,
                        TString& articleName,
                        TString& answerBody)
{
    // This means that the query probably has nothing to do with definitions
    if (!OnlyMarkerMarker(query, normalizeInfos)) {
        return false;
    }

    TUtf16String wtext = UTF8ToWide(text);

    // Trying to find the very first delimiter and determine its bounds for a proper split.
    // Important: the delimiter must be oustide of all pairs of brackets.
    int balance = 0;
    size_t i = 0;
    size_t firstDelimiterBegin = TUtf16String::npos;
    size_t firstDelimiterEnd = TUtf16String::npos;

    for (auto it = wtext.begin(); it != wtext.end(); ++it) {
        const wchar16 wch = *it;

        if (wch == '(') {
            ++balance;
        } else if (wch == ')') {
            --balance;
        } else if (balance == 0) {
            TUtf16String currentSuffix = wtext.substr(i, wtext.size() - i);

            for (const TUtf16String& delimiter: DEFINITION_DELIMITERS) {
                size_t len = delimiter.size();

                if (currentSuffix.size() >= len) {
                    if (TWtringBuf(currentSuffix.data(), len) == delimiter) {
                        firstDelimiterBegin = i;
                        firstDelimiterEnd = i + delimiter.size() - 1;
                        break;
                    }
                }
            }

            if (firstDelimiterBegin != TUtf16String::npos) {
                break;
            }
        }

        ++i;
    }

    if (firstDelimiterBegin == TUtf16String::npos) {
        return false;
    }

    TUtf16String wideArticleName = PrettifyArticleName(wtext.substr(0, firstDelimiterBegin));

    if (wideArticleName.size() > MAX_LETTERS_IN_HEADLINE) {
        return false;
    }

    size_t spaceCount = 0;
    for (auto it = wideArticleName.begin(); it != wideArticleName.end(); ++it) {
        const wchar16 wch = *it;
        if (IsBlank(wch) || IsSpace(wch)) {
            ++spaceCount;
        }
    }

    if (spaceCount + 1 > MAX_WORDS_IN_HEADLINE) {
        return false;
    }

    // Check if the query is actually the notion being defined plus
    // a corresponding marker -- after application of at least one of given normalizations
    bool matchingNormalizationExists = false;

    for (const THolder<TNormalizeByLemmasInfo>& normalizeInfo: normalizeInfos) {
        if (!OnlyMarkerMarker(query, normalizeInfo)) {
            continue;
        }

        TString normQueryBody = NormalizeByLemmasUTF8(query, *(normalizeInfo.Get()));
        size_t markerPosition = normQueryBody.find(MARKER_MARKER);
        if (markerPosition != TString::npos) {
            normQueryBody = StripString(normQueryBody.substr(0, markerPosition));
        }

        TString normalizedArticleName = NormalizeByLemmasUTF8(wideArticleName, *(normalizeInfo.Get()));
        if (normQueryBody == normalizedArticleName) {
            matchingNormalizationExists = true;
            break;
        }
    }

    if (!matchingNormalizationExists) {
        return false;
    }

    TUtf16String wideAnswerBody = StripString(wtext.substr(firstDelimiterEnd + 1, wtext.size() - firstDelimiterEnd - 1));

    // Capitalize the first symbol of the answer body
    wideAnswerBody.to_upper(0, 1);

    articleName = WideToUTF8(wideArticleName);
    answerBody = WideToUTF8(wideAnswerBody);

    return true;
}
