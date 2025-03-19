#include "form_index_assigner.h"

#include <util/generic/xrange.h>

namespace {
    // Comparison operations influence the outer world, namely RichTreeFormId and LowLevelFormId.
    // It isn't important to use a particular ordering, but once selected,
    // changing the ordering should be done with care.

    // Standard operator< on TWtringBuf is lexicographical and performs poorly
    // on forms of the same lemma (with large common prefix and difference in last characters).
    // Thus, order by length and resort to data comparison only within same length.
    // memcmp isn't the same as wchar16 comparison; it is still a valid operator<, so we don't care.
    struct TLengthFirstCompare
    {
        bool operator()(TWtringBuf l, TWtringBuf r) const {
            if (l.size() != r.size())
                return l.size() < r.size();
            return (memcmp(l.data(), r.data(), l.size() * sizeof(wchar16)) < 0);
        }
    };
} // namespace

namespace NReqBundleIteratorImpl {
    TFormIndexAssigner::TFormIndexAssigner(
        NReqBundle::TConstWordAcc word,
        TMemoryPool& pool)
        : Form2Id(&pool)
        , Id2Form(&pool)
    {
        Id2Form.reserve(2 + word.GetTotalNumForms());

        Id2Form.push_back(TWtringBuf()); // special zero forma
        Id2Form.push_back(pool.AppendString<TUtf16String::char_type>(word.GetWideText()));
        for (auto lemma : word.GetLemmas())  {
            for (auto form : lemma.GetForms()) {
                Id2Form.push_back(pool.AppendString<TUtf16String::char_type>(form.GetWideText()));
            }
        }

        Sort(Id2Form.begin() + 1, Id2Form.end(), TLengthFirstCompare());
        const size_t numUniqForms = Unique(Id2Form.begin() + 1, Id2Form.end()) - Id2Form.begin();
        Id2Form.resize(numUniqForms);
        Y_ENSURE(Id2Form.size() <= Max<ui16>(), "too many forms in richtree node");

        Form2Id.reserve(Id2Form.size() - 1);
        for (size_t i : xrange<size_t>(1, Id2Form.size())) {
            Form2Id[Id2Form[i]] = ui16(i);
        }
    }
} // NReqBundleIteratorImpl
