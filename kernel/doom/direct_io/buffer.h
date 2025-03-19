#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NDoom {

template<typename T>
class TRingArray {
public:
    TRingArray(size_t size)
        : Storage_(size + 1)
        , Size_(size + 1)
    {
    }

    void PushBack(T val) {
        Storage_[End_] = std::move(val);
        ++End_;
        if (End_ >= Size_) {
            End_ -= Size_;
        }
        Y_VERIFY(End_ != Start_);
    }

    void Advance(size_t count) {
        Start_ += count;
        while (Start_ >= Size_) {
            Start_ -= Size_;
        }
    }

    TArrayRef<T> GetChunk() {
        if (Start_ == End_) {
            return {};
        } else if (Start_ < End_) {
            return { &Storage_[Start_], End_ - Start_ };
        } else {
            return { &Storage_[Start_], Size_ - Start_ };
        }
    }

    bool Empty() {
        return Start_ == End_;
    }

    void Clear() {
        Start_ = End_ = 0;
    }

    T& Head() {
        return Storage_[Start_];
    }

private:
    TVector<T> Storage_;
    size_t Size_;
    size_t Start_ = 0;
    size_t End_ = 0;
};

} // namespace NDoom
