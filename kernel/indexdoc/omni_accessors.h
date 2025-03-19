#pragma once

#include "omnidoc.h"
#include "doc_omni_accessor.h"

#include <kernel/doom/offroad_struct_wad/doc_data_holder.h>


namespace NOmniAccessorsDetail {

template <class T>
struct TGetAccessor {
    using TType = THolder<TDocOmniIndexAccessor<T>>;
};

template <class Io>
struct TSearcherSelector {
    static THolder<TDocOmniIndexAccessor<Io>> Create(const TDocOmniWadIndex* index) {
        return TOmniAccessorFactory::NewAccessor<Io>(index);
    }

    static THolder<TDocOmniIndexAccessor<Io>> Create(const TDocOmniWadIndex* index, const TSearchDocDataHolder* docDataHolder) {
        return TOmniAccessorFactory::NewAccessor<Io>(index, docDataHolder);
    }
};

}

class TOmniAccessors {
    using TAllIos = TDocOmniWadIndex::TAllIos;
    using TAccessors = TRebindPackWithSubtype<std::tuple, TAllIos, NOmniAccessorsDetail::TGetAccessor>;

public:
    TOmniAccessors(const TDocOmniWadIndex* index)
        : DocDataHolder_(MakeHolder<TSearchDocDataHolder>())
    {
        Accessors_ = TAccessors::InitWithFactory<NOmniAccessorsDetail::TSearcherSelector>(index, DocDataHolder_.Get());
    }

    template<class Io>
    TDocOmniIndexAccessor<Io>* GetAccessor() const {
        constexpr size_t accessorIndex = TIndexOfType<Io, TAllIos>::Value;
        return std::get<accessorIndex>(Accessors_).Get();
    }

    template<typename Loader>
    void PreLoadDoc(ui32 docId, Loader&& loader) {
        DocDataHolder_->PreLoad(docId, std::move(loader));
    }

    void RemoveDoc(ui32 docId) {
        DocDataHolder_->Remove(docId);
    }

protected:
    THolder<TSearchDocDataHolder> DocDataHolder_;

private:
    TAccessors::TType Accessors_;
};
