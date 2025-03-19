#include <library/cpp/testing/unittest/registar.h>

#include <random>

#include "decoding_index_reader.h"
#include "encoding_index_writer.h"
#include "multi_key_index_writer.h"

#include <kernel/doom/hits/superlong_hit.h>
#include <kernel/doom/key/key_decoder.h>
#include <kernel/doom/key/key_encoder.h>
#include <kernel/doom/simple_map/simple_map_reader.h>
#include <kernel/doom/simple_map/simple_map_writer.h>

#include <util/generic/map.h>
#include <util/stream/file.h>
#include <util/system/tempfile.h>

using namespace NDoom;

Y_UNIT_TEST_SUITE(TMultiKeyIndexWriter) {

    using TSuperlongHit = ::TSuperlongHit;

    struct TIndexEntry {
        TDecodedKey Key;
        TVector<TSuperlongHit> Hits;

        TIndexEntry() = default;

        TIndexEntry(TDecodedKey key, const TVector<TSuperlongHit>& hits)
            : Key(key)
            , Hits(hits)
        {
        }
    };

    bool operator==(const TIndexEntry& l, const TIndexEntry& r) {
        return l.Key == r.Key && l.Hits == r.Hits;
    }

    IOutputStream& operator<<(IOutputStream& out, const TIndexEntry& entry) {
        out << "Key: " << entry.Key << ", Hits: [";
        for (size_t i = 0; i < entry.Hits.size(); ++i) {
            if (i > 0) {
                out << ", ";
            }
            out << entry.Hits[i];
        }
        out << "]";
        return out;
    }

    struct TIndex {
        TVector<TIndexEntry> EntriesToWriter;
        TVector<TIndexEntry> EntriesExpectedFromReader;
    };

    void SortAndUniqueHits(TVector<TSuperlongHit>& hits) {
        Sort(hits.begin(), hits.end());
        hits.erase(Unique(hits.begin(), hits.end()), hits.end());
    }

    TVector<size_t> RenumerateFormsSortAndUniqueHits(TVector<TSuperlongHit>& hits, const TVector<TDecodedFormRef>& allForms) {
        TVector<size_t> formHitsCount(allForms.size(), 0);
        for (const auto& hit : hits) {
            Y_ASSERT(hit.Form() < formHitsCount.size());
            ++formHitsCount[hit.Form()];
        }
        TVector<size_t> formIndexes(allForms.size());
        std::iota(formIndexes.begin(), formIndexes.end(), 0);
        auto cmp = [&] (size_t l, size_t r) {
            if (formHitsCount[l] != formHitsCount[r]) {
                return formHitsCount[l] > formHitsCount[r];
            }
            return allForms[l] < allForms[r];
        };
        Sort(formIndexes.begin(), formIndexes.end(), cmp);
        TVector<size_t> invertedFormIndexes(allForms.size());
        for (size_t i = 0; i < allForms.size(); ++i) {
            invertedFormIndexes[formIndexes[i]] = i;
        }
        for (auto& hit : hits) {
            ui32 form = hit.Form();
            Y_ASSERT(form < invertedFormIndexes.size());
            hit.SetForm(invertedFormIndexes[form]);
        }
        SortAndUniqueHits(hits);
        return formIndexes;
    }

    TIndex GetSimpleIndex() {
        TIndex index;
        TDecodedKey key;

        // entries to writer
        key.SetLemma("Lemma1");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form1");
        key.AddForm(LANG_ENG, FORM_HAS_LANG | FORM_TRANSLIT, "Form1");
        key.AddForm(LANG_AZE, FORM_HAS_LANG, "Forma1");
        index.EntriesToWriter.push_back(TIndexEntry(key, { TSuperlongHit(1, 1, 1, 1, 0), TSuperlongHit(1, 1, 1, 1, 1), TSuperlongHit(2, 1, 2, 2, 2) }));

        key.Clear();
        key.SetLemma("Lemma1");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form2");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form1");
        index.EntriesToWriter.push_back(TIndexEntry(key, { TSuperlongHit(1, 1, 1, 1, 1), TSuperlongHit(42, 2, 5, 0, 0) }));

        key.Clear();
        key.SetLemma("Lemma2");
        key.AddForm(LANG_UNK, EFormFlags(), "Lemma2");
        index.EntriesToWriter.push_back(TIndexEntry(key, { TSuperlongHit(1, 1, 1, 1, 0) }));

        key.Clear();
        key.SetLemma("Lemma3");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form1");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form2");
        index.EntriesToWriter.push_back(TIndexEntry(key, { TSuperlongHit(2, 1, 1, 1, 0), TSuperlongHit(2, 1, 1, 2, 1) }));

        key.Clear();
        key.SetLemma("Lemma3");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form2");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form3");
        index.EntriesToWriter.push_back(TIndexEntry(key, { TSuperlongHit(13, 1, 2, 3, 0), TSuperlongHit(13, 2, 3, 1, 1) }));

        // entries expected from reader
        key.Clear();
        key.SetLemma("Lemma1");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form1");
        key.AddForm(LANG_ENG, FORM_HAS_LANG | FORM_TRANSLIT, "Form1");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form2");
        key.AddForm(LANG_AZE, FORM_HAS_LANG, "Forma1");
        TVector<TSuperlongHit> hits;
        hits.emplace_back(1, 1, 1, 1, 0);
        hits.emplace_back(1, 1, 1, 1, 1);
        hits.emplace_back(2, 1, 2, 2, 3);
        hits.emplace_back(1, 1, 1, 1, 0);
        hits.emplace_back(42, 2, 5, 0, 2);
        SortAndUniqueHits(hits);
        index.EntriesExpectedFromReader.push_back(TIndexEntry(key, hits));

        key.Clear();
        key.SetLemma("Lemma2");
        key.AddForm(LANG_UNK, EFormFlags(), "Lemma2");
        index.EntriesExpectedFromReader.push_back(TIndexEntry(key, { TSuperlongHit(1, 1, 1, 1, 0) }));

        key.Clear();
        key.SetLemma("Lemma3");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form2");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form1");
        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form3");
        hits.clear();
        hits.emplace_back(2, 1, 1, 1, 1);
        hits.emplace_back(2, 1, 1, 2, 0);
        hits.emplace_back(13, 1, 2, 3, 0);
        hits.emplace_back(13, 2, 3, 1, 2);
        SortAndUniqueHits(hits);
        index.EntriesExpectedFromReader.push_back(TIndexEntry(key, hits));

        return index;
    }

    TIndex GetRandomIndex() {
        TIndex index;
        std::minstd_rand random(4243);
        const TVector<TString> lemmas = { "Lemma", "Abaca", "Bacaba", "Cabaca", "Abada", "Dada" };
        TVector<size_t> formsCount(lemmas.size(), 0);
        const TVector<TString> forms = { "Form", "Forma", "Yandex", "Search" };
        const TVector<ELanguage> langs = { LANG_RUS, LANG_ENG, LANG_UNK, LANG_AZE };
        const size_t keysCount = 88;
        for (size_t i = 0; i < keysCount; ++i) {
            size_t lemmInd = random() % lemmas.size();
            if (formsCount[lemmInd] >= 16) {
                continue;
            }
            TDecodedKey key;
            key.SetLemma(lemmas[lemmInd]);
            size_t curLemmForms = 1 + random() % (16 - formsCount[lemmInd]);
            formsCount[lemmInd] += curLemmForms;
            for (size_t j = 0; j < curLemmForms; ++j) {
                size_t formInd = random() % forms.size();
                ELanguage lang = langs[random() % langs.size()];
                EFormFlags flags;
                if (lang != LANG_UNK) {
                    flags |= FORM_HAS_LANG;
                }
                if (random() % 100 < 10) {
                    flags |= FORM_TRANSLIT;
                }
                key.AddForm(lang, flags, forms[formInd]);
            }
            TVector<TSuperlongHit> hits;
            size_t hitsCount = 1 + random() % 30;
            for (size_t j = 0; j < hitsCount; ++j) {
                hits.emplace_back(1 + random() % 10, 1 + random() % 10, 1 + random() % 10, random() % 4, random() % curLemmForms);
            }
            SortAndUniqueHits(hits);
            index.EntriesToWriter.emplace_back(key, hits);
        }
        SortBy(index.EntriesToWriter.begin(), index.EntriesToWriter.end(), [](const TIndexEntry& entry) -> TStringBuf { return entry.Key.Lemma(); });
        for (size_t i = 0; i < index.EntriesToWriter.size(); ) {
            size_t j = i;
            while (j < index.EntriesToWriter.size() && index.EntriesToWriter[i].Key.Lemma() == index.EntriesToWriter[j].Key.Lemma()) {
                ++j;
            }

            TVector<TDecodedFormRef> allForms;
            for (size_t z = i; z < j; ++z) {
                for (size_t h = 0; h < index.EntriesToWriter[z].Key.FormCount(); ++h) {
                    allForms.push_back(index.EntriesToWriter[z].Key.Form(h));
                }
            }
            Sort(allForms.begin(), allForms.end());
            allForms.erase(Unique(allForms.begin(), allForms.end()), allForms.end());

            TVector<TSuperlongHit> hits;
            for (size_t z = i; z < j; ++z) {
                for (const TSuperlongHit& hit : index.EntriesToWriter[z].Hits) {
                    hits.push_back(hit);
                    auto it = Find(allForms.begin(), allForms.end(), index.EntriesToWriter[z].Key.Form(hit.Form()));
                    Y_ASSERT(it != allForms.end());
                    hits.back().SetForm(it - allForms.begin());
                }
            }

            TVector<size_t> formIndexes = RenumerateFormsSortAndUniqueHits(hits, allForms);

            TDecodedKey key;
            key.SetLemma(index.EntriesToWriter[i].Key.Lemma());
            for (size_t formIndex: formIndexes) {
                key.AddForm(allForms[formIndex]);
            }

            index.EntriesExpectedFromReader.emplace_back(key, hits);
            i = j;
        }
        return index;
    }

    template <class TWriter, class TReader>
    void TestWriteAndRead(const TIndex& index) {
        typename TWriter::TIndexMap storage;

        TMultiKeyIndexWriter<TEncodingIndexWriter<TWriter, TKeyEncoder>> writer(storage);
        for (const TIndexEntry& entry : index.EntriesToWriter) {
            for (const TSuperlongHit& hit : entry.Hits) {
                writer.WriteHit(hit);
            }
            writer.WriteKey(entry.Key);
        }
        writer.Finish();

        TDecodingIndexReader<TReader, TKeyDecoder> reader(EKeyDecodingOption::NoDecodingOptions, storage);
        TDecodedKey key;
        TMap<TDecodedKey, TVector<TSuperlongHit>> hitsByKey;
        while (reader.ReadKey(&key)) {
            TSuperlongHit hit;
            while (reader.ReadHit(&hit)) {
                hitsByKey[key].push_back(hit);
            }
        }
        TVector<TIndexEntry> entries;
        for (const auto& entry : hitsByKey) {
            entries.emplace_back(entry.first, entry.second);
        }
        UNIT_ASSERT_VALUES_EQUAL(index.EntriesExpectedFromReader, entries);
    }

    Y_UNIT_TEST(WriteAndReadSimpleIndex) {
        TIndex index = GetSimpleIndex();
        TestWriteAndRead<TSimpleMapWriter<TSuperlongHit>, TSimpleMapReader<TSuperlongHit>>(index);
    }

    Y_UNIT_TEST(WriteAndReadRandomIndex) {
        TIndex index = GetRandomIndex();
        TestWriteAndRead<TSimpleMapWriter<TSuperlongHit>, TSimpleMapReader<TSuperlongHit>>(index);
    }
}
