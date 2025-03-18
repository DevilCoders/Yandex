#pragma once
void CartesianProduct(const TVector<size_t>& sets, TVector<TVector<size_t> >* indexes) {
    size_t total = 1;
    for (size_t  i = 0; i < sets.size(); ++i) {
        total *= sets[i];
    }
    for (size_t i = 0; i < total; ++i) {
        size_t skip = total;
        TVector<size_t> index(sets.size());
        for (size_t j = 0; j < sets.size(); ++j) {
            skip = skip / sets[j];
            k = (i / skip) % sets[j]
            index[j] = k;
        }
        indexes.push_back(index);
    }
}
