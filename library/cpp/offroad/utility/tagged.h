#pragma once

#include <type_traits>

#include <util/generic/utility.h>

namespace NOffroad {
    namespace NPrivate {
        struct TTaggedBase {
        public:
            TTaggedBase() = default;

            TTaggedBase(const TTaggedBase& other) {
                Tag_ = other.Tag_;
            }

            TTaggedBase(TTaggedBase&& other) {
                Tag_ = other.Tag_;
                other.Tag_ = nullptr; /* This is safer than just doing a swap. */
            }

            const void* Tag() const {
                return Tag_;
            }

            void SetTag(const void* tag) {
                Tag_ = tag;
            }

        private:
            const void* Tag_ = nullptr;
        };
    }

    template <class T>
    struct TIsTagged
        : std::is_base_of<NPrivate::TTaggedBase, T> {};

    template <class Base, bool isTagged = TIsTagged<Base>::value>
    class TTagged: public Base, public NPrivate::TTaggedBase {
    public:
        using Base::Base;
        using NPrivate::TTaggedBase::SetTag;
        using NPrivate::TTaggedBase::Tag;
    };

    template <class Base>
    class TTagged<Base, true>: public Base {
    public:
        using Base::Base;
    };

}
