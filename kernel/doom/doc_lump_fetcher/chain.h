#pragma once

#include "doc_lump_fetcher.h"

#include <kernel/doom/wad/multi_mapper.h>

namespace NDoom {

template<typename BaseLoader>
class TChainedFetchersWrapper : public IDocLumpFetcher<TMultiDocLumpLoader<BaseLoader>> {
public:
    TChainedFetchersWrapper() = default;

    TChainedFetchersWrapper<BaseLoader>& AddFetcher(const IDocLumpFetcher<BaseLoader>& fetcher) {
        Y_ENSURE(!MultiFetcher_);
        Fetchers_.push_back(&fetcher);
        BaseMappers_.push_back(&fetcher.Mapper());
        return *this;
    }

    TChainedFetchersWrapper<BaseLoader>& AddOwnFetcher(THolder<IDocLumpFetcher<BaseLoader>>&& fetcher) {
        FetchersHolders_.push_back(std::move(fetcher));
        return AddFetcher(*FetchersHolders_.back());
    }

    template<typename BaseMapper>
    TChainedFetchersWrapper<BaseLoader>& SetMultiFetcher(const IDocLumpFetcher<TMultiDocLumpLoader<BaseLoader>>& multiFetcher, const TMultiDocLumpMapper<BaseMapper>& multiMapper) {
        Y_ENSURE(!MultiFetcher_);
        MultiFetcher_ = &multiFetcher;
        for (auto mapper : multiMapper.Mappers()) {
            BaseMappers_.push_back(mapper);
        }
        return *this;
    }

    template<typename BaseMapper>
    TChainedFetchersWrapper<BaseLoader>& SetOwnMultiFetcher(THolder<IDocLumpFetcher<TMultiDocLumpLoader<BaseLoader>>>&& multiFetcher,
                                                            const TMultiDocLumpMapper<BaseMapper>& multiMapper)
    {
        MultiFetcherHolder_.Reset(multiFetcher.Release());
        return SetMultiFetcher(*MultiFetcherHolder_, multiMapper);
    }

    TChainedFetchersWrapper<BaseLoader>& Init() {
        Y_ENSURE(!Mapper_);
        Mapper_ = MakeHolder<TMultiDocLumpMapper<>>(BaseMappers_);
        return *this;
    }

    TUnanswersStats Fetch(TConstArrayRef<ui32> ids, std::function<void(size_t, TMultiDocLumpLoader<BaseLoader>*)> cb) const override {
        Y_VERIFY(Mapper_);
        TVector<TVector<BaseLoader>> loaders(ids.size());
        for (auto& loaderVector : loaders) {
            loaderVector = TVector<BaseLoader>(BaseMappers_.size());
        }

        TVector<bool> success(ids.size(), true);
        TUnanswersStats result{ static_cast<ui32>(ids.size()), 0 };
        auto ret = [&](size_t j) {
            if (success[j]) {
                TMultiDocLumpLoader<BaseLoader> loader = Mapper_->GetLoader(std::move(loaders[j]));
                cb(j, &loader);
            } else {
                cb(j, nullptr);
                ++result.NumUnanswers;
            }
        };
        for (size_t i = 0; i < Fetchers_.size(); ++i) {
            Fetchers_[i]->Fetch(
                ids,
                [&](size_t j, const BaseLoader* loader) {
                    if (loader) {
                        loaders[j][i] = *loader;
                    } else {
                        success[j] = false;
                    }
                    if (i + 1 == Fetchers_.size() && !MultiFetcher_) {
                        ret(j);
                    }
                });
        }

        if (MultiFetcher_) {
            MultiFetcher_->Fetch(
                ids,
                [&](size_t j, TMultiDocLumpLoader<BaseLoader>* loader) {
                    if (loader) {
                        auto multiLoaders = loader->BaseLoaders();
                        for (size_t i = 0; i < multiLoaders.size(); ++i) {
                            loaders[j][Fetchers_.size() + i] = std::move(multiLoaders[i]);
                        }
                    } else {
                        success[j] = false;
                    }
                    ret(j);
                });
        }
        return result;
    }

    const IDocLumpMapper& Mapper() const override {
        return *Mapper_;
    }

    const TMultiDocLumpMapper<> MultiMapper() const {
        return *Mapper_;
    }

private:
    TVector<const IDocLumpFetcher<BaseLoader>*> Fetchers_;
    TVector<THolder<IDocLumpFetcher<BaseLoader>>> FetchersHolders_;

    TVector<const IDocLumpMapper*> BaseMappers_;
    THolder<TMultiDocLumpMapper<>> Mapper_;

    const IDocLumpFetcher<TMultiDocLumpLoader<BaseLoader>>* MultiFetcher_ = nullptr;
    THolder<IDocLumpFetcher<TMultiDocLumpLoader<BaseLoader>>> MultiFetcherHolder_ = nullptr;
};

}
