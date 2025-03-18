#pragma once

#include <utility>
#include <tuple>
#include <type_traits>

namespace NOffroad {
    template <class T>
    struct TMaterializedType {
        using type = T;
    };

    template <class T>
    struct TMaterializedType<T&&> {
        using type = typename TMaterializedType<T>::type;
    };

    template <class T>
    struct TMaterializedType<T&> {
        using type = typename TMaterializedType<T>::type;
    };

    template <class... T>
    struct TMaterializedType<std::tuple<T...>> {
        using type = std::tuple<typename TMaterializedType<T>::type...>;
    };

    template <class T>
    typename TMaterializedType<T>::type Materialize(T&& value) {
        return {std::forward<T>(value)};
    }

}
