#pragma once

#include <utility>

namespace NOffroad {
    template <class T>
    class TResetter {
    public:
        TResetter(T* target)
            : Target_(target)
        {
        }

        template <class... Args>
        void operator()(Args&&... args) {
            Target_->Reset(std::forward<Args>(args)...);
        }

    private:
        T* Target_;
    };

    template <class T>
    TResetter<T> MakeResetter(T* target) {
        return {target};
    }

}
