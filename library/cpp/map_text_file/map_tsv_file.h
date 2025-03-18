#pragma once

#include <util/generic/vector.h>
#include <util/generic/strbuf.h>

#include "map_text_file.h"
#include <util/string/split.h>

template <typename FieldSplitter>
class TSvIter : std::iterator<std::forward_iterator_tag, TVector<TStringBuf>> {
public:
    TSvIter(TIterTextLines base, int requiredFields)
        : Impl(base)
        , RequiredFields(requiredFields)
    {
        Split();
    }

    TSvIter() {
    }

    bool operator==(const TSvIter& r) const {
        return (Impl == r.Impl);
    }

    bool operator!=(const TSvIter& r) const {
        return !(*this == r);
    }

    void operator++() {
        Impl.Next();
        Split();
    }

    const TVector<TStringBuf>& operator*() const {
        return Tabs;
    }

private:
    void Split() {
        Tabs.clear();
        if (!Impl.Valid())
            return;
        FieldSplitter::LineToFieldVec(*Impl, &Tabs);

        if (RequiredFields != -1 && static_cast<size_t>(RequiredFields) != Tabs.size())
            ythrow yexception() << "Invalid number of fields in TSV file, " << Tabs.size() << " != " << RequiredFields << " in line \"" << *Impl << "\"";
    }

private:
    TIterTextLines Impl;
    TVector<TStringBuf> Tabs;
    int RequiredFields;
};

template <typename FieldSplitter>
class TMapSvFile {
public:
    /**
     * @param path                      Path to tsv-like file to open.
     * @param requiredFields            Expected number of fields in a single line.
     *                                  If a line with a different number of fields is encountered,
     *                                  an exception will be thrown. Use `-1` to disable this check.
     */
    TMapSvFile(const TString& path, int requiredFields = -1)
        : Impl(path)
        , RequiredFields(requiredFields)
    {
        Y_ENSURE(requiredFields >= -1, "invalid TMapTextFile(requiredFields) arg");
    }

    auto begin() const {
        return TSvIter<FieldSplitter>(Impl.begin(), RequiredFields);
    }

    auto end() const {
        return TSvIter<FieldSplitter>();
    }

    size_t LineCount() {
        return Impl.LineCount();
    }

private:
    TMapTextFile Impl;
    int RequiredFields;
};

namespace NPrivate {
    struct DefaultFieldSplitter {
        Y_FORCE_INLINE static void LineToFieldVec(TStringBuf line, TVector<TStringBuf>* resultFields) {
            StringSplitter(line).Split('\t').AddTo(resultFields);
        }
    };
}

/**
 * Iterate over split lines of a TSV (tab-separated values) file.
 *
 *     for (const TVector<TStringBuf>& tabs: TMapTsvFile("config.tsv")) {
 *         if (tabs.size() < 4)
 *             throw yexception();
 *         ...
 *     }
 */
using TMapTsvFile = TMapSvFile<NPrivate::DefaultFieldSplitter>;

using TTsvIter = TSvIter<NPrivate::DefaultFieldSplitter>;
