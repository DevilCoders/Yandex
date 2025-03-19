#pragma once

#include <kernel/searchlog/errorlog.h>
#include <kernel/doom/offroad_omni_wad/omni_io.h>

#include <util/generic/ptr.h>
#include <util/generic/typelist.h>
#include <util/generic/cast.h>
#include <array>
#include <tuple>
#include <type_traits>

#include "omnidoc_fwd.h"


template<class T, class TypeList>
struct TIndexOfType {
    static_assert(TypeList::template THave<T>::value, "No requested type in typelist");
};

template <class T,  class... Types>
struct TIndexOfType<T, TTypeList<T, Types...>> {
    static constexpr size_t Value = 0;
};

template <class T, class U, class ...Types>
struct TIndexOfType<T, TTypeList<U, Types...>> {
    static constexpr size_t Value = 1 + TIndexOfType<T, TTypeList<Types...>>::Value;
};


template<class T, int Tag_>
struct TTypeTag {
    using TType = T;
    static constexpr int Tag = Tag_;
};


template<class T, class TypeList>
struct TTagOfType {
    static constexpr int Tag = -1; //No type in Typelist
};

template <class T,  class... Types, int Tag_>
struct TTagOfType<T, TTypeList<TTypeTag<T, Tag_>, Types...>> {
    static constexpr int Tag = Tag_;
};

template <class T, class U, class ...Types, int Tag_>
struct TTagOfType<T, TTypeList<TTypeTag<U, Tag_>, Types...>> {
    static constexpr int Tag = TTagOfType<T, TTypeList<Types...>>::Tag;
};

template<class T, class TypeList>
struct TTagOfTypeList {
    static constexpr int Tag = -1; //No type in Typelist
};

template <class T, class ...Types>
struct TTagOfTypeList<TTypeList<T>, TTypeList<Types...>> {
    static constexpr int Tag = TTagOfType<T, TTypeList<Types...>>::Tag;
};

template <class T, class... Ts, class ...Us>
struct TTagOfTypeList<TTypeList<T, Ts...>, TTypeList<Us...>> {
    static constexpr int Tag = (TTagOfTypeList<TTypeList<T>, TTypeList<Us...>>::Tag > 0)
                                    ? TTagOfTypeList<TTypeList<T>, TTypeList<Us...>>::Tag
                                    : TTagOfTypeList<TTypeList<Ts...>, TTypeList<Us...>>::Tag;
};

// TODO(tender-bum): remove this
namespace NDisjunction {
    template<bool ...> struct TDisjunction;

    template<bool B> struct TDisjunction<B>  {
        static constexpr bool Value = B;
    };

    template<bool B1, bool... Bn>
    struct TDisjunction<B1, Bn...> {
        static constexpr bool Value = std::conditional<B1, TDisjunction<B1>, TDisjunction<Bn...>>::type::Value;
    };
}


template <class T, class Typelist>
struct TTypeListHasMatchingHead {
    static constexpr bool Value = false;
};

template <class T, class... Ts>
struct TTypeListHasMatchingHead<T, TTypeList<Ts...>> {
    // TODO(tender-bum): remove this
    static constexpr bool Value = NDisjunction::TDisjunction<std::is_same<typename Ts::THead,T>::value...>::Value;
};


template <class, class>
struct TMatchTypeListInTypeListByHead;

template<class T, class...Ts>
struct TMatchTypeListInTypeListByHead<T,TTypeList<Ts...>> {
    static_assert(TTypeListHasMatchingHead<T, Ts...>::Value, "No matching Head in for requested type in typelist");
};

template <class T,  class... TypeLists, class... Ts>
struct TMatchTypeListInTypeListByHead<T, TTypeList<TTypeList<T, Ts...>, TypeLists...>> {
    using TType = TTypeList<Ts...>;
};

template <class T,  class U, class... TypeLists, class... Ts>
struct TMatchTypeListInTypeListByHead<T, TTypeList<TTypeList<U, Ts...>, TypeLists...>> {
    using TType = typename TMatchTypeListInTypeListByHead<T, TTypeList<TypeLists...>>::TType;
};


template <class... TypeLists>
struct TTypeListCat;

template <class... TypeLists, class... Ts>
struct TTypeListCat<TTypeList<Ts...>, TypeLists...> {
    using TType = typename TTypeListCat<TTypeList<Ts...>, typename TTypeListCat<TypeLists...>::TType>::TType;
};

template <class... Ts, class... Us>
struct TTypeListCat<TTypeList<Ts...>, TTypeList<Us...>> {
    using TType = TTypeList<Ts..., Us...>;
};


template<template<typename...>class Target, typename Src>
struct TRebindPackFromHoldersTuple;

template<template<typename...>class Target, template<typename...>class Src, typename... Ts>
struct TRebindPackFromHoldersTuple<Target, Src<Ts...>> {
    using TType = Target<typename Ts::TValueType...>;
};


template<template<typename...>class Target, typename Src, template<typename> class Subber>
struct TRebindPackWithSubtype;

template<template<typename...>class Target, template<typename> class Subber, template<typename...>class Src, typename... Ts>
struct TRebindPackWithSubtype< Target, Src<Ts...>, Subber > {
    using TType = Target<typename Subber<Ts>::TType...>;
    using TBases = TTypeList<Ts...>;

    template<class... Args>
    static TType Init(Args&&... args) {
        return Target<typename Subber<Ts>::TType...>(ApplySimple<Ts>(std::forward<Args>(args)...)...);
    }

    template<template <typename> class Factory, class... Args>
    static TType InitWithFactory(Args&&... args) {
        return Target<typename Subber<Ts>::TType...>(ApplyFactory<Factory, Ts>(std::forward<Args>(args)...)...);
    }

    template<class... Args>
    static TType InitHolders(Args&&... args) {
        return Target<typename Subber<Ts>::TType...>(ApplyHolder<Ts>(std::forward<Args>(args)...)...);
    }

private:
    template<class Io>
    static void PrintWarning() {
        static constexpr NDoom::EWadIndexType indexType = Io::IndexType; // can't take address for `<<`
        SEARCH_WARNING << "No model " << ToString(indexType) << " found.";
    }

    template<class Io, class... Args>
    static typename Subber<Io>::TType ApplyHolder(Args&&... args) {
        try {
            return typename Subber<Io>::TType(new typename Subber<Io>::TType::TValueType{std::forward<Args>(args)...});
        } catch (...) {
            PrintWarning<Io>();
        }
        return nullptr;
    }

    template<template <typename> class Factory, class Io, class... Args>
    static typename Subber<Io>::TType ApplyFactory(Args&&... args) {
        try {
            return Factory<Io>::Create(std::forward<Args>(args)...);
        } catch (...) {
            PrintWarning<Io>();
        }
        return nullptr;
    }

    template<class Io, class... Args>
    static typename Subber<Io>::TType ApplySimple(Args&&... args) {
        try {
            return typename Subber<Io>::TType(std::forward<Args>(args)...);
        } catch (...) {
            PrintWarning<Io>();
        }
        return nullptr;
    }
};
