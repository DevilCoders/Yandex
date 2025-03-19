#pragma once

#include <ysite/yandex/reqanalysis/normalize.h>
#include <kernel/urlnorm/normalize.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/split.h>
#include <util/string/printf.h>
#include <util/string/join.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/generic/ymath.h>
#include <util/generic/set.h>
#include <util/generic/utility.h>
#include <util/stream/file.h>

namespace NUrlTranslitSimilarity {
    // значение функции больше равно числу букв в utf8 строке
    // используется для оценки хорошести строки
    // и для ограничения перебора
    inline size_t PosUtf8Length(TStringBuf str) {
        size_t length = 1;
        try {
            length = GetNumberOfUTF8Chars(str);
        } catch (...) {
            length = str.length();
        }
        return length;
    }

    inline size_t GetMaxUtf8LengthBound(const TVector<TString>& words) {
        size_t maxLength = 1;
        for (const auto& ts : words) {
            maxLength = Max(PosUtf8Length(ts), maxLength);
        }
        return maxLength;
    }

    inline TVector<TString> SplitStrByWords(const TString& str) {
        const TStringBuf delimiters = TStringBuf("/\\;~!#$^&?><().,'\"][}{=-+_| ");
        TVector<TString> words1;
        StringSplitter(str).SplitBySet(delimiters.data()).AddTo(&words1);
        TVector<TString> words2;
        for (const auto& tw : words1) {
            if (!tw.empty()) {
                words2.push_back(tw);
            }
        }
        return words2;
    }

    struct TUrlWords {
        TString Path;
        TString Host;
        TVector<TString> PathWords;
        TVector<TString> HostWords;

        TUrlWords(const TString& url) {
            TString unesc;
            NUrlNorm::NormalizeUrl(url, unesc);
            UrlUnescape(unesc);
            SplitUrlToHostAndPath(unesc, Host, Path);
            Host = CutWWWPrefix(CutSchemePrefix(Host));
            PathWords = SplitStrByWords(Path);
            HostWords = SplitStrByWords(Host);
        }
    };

    // Если есть уверенность, что во входных файлах нет виндусовых концов строк,
    // то эта функция не нужна
    // только для тестирования
    // когда будет уверенность, что данные правильные, это можно будет убрать
    inline void TrimEolSpace(TString& str) {
        const TString dropChars(" \t\n\r");

        int pos = str.length();
        while ((pos > 0) && (TString::npos != dropChars.find(str[pos - 1]))) {
            pos--;
        }
        str.erase(pos);
    }
}
