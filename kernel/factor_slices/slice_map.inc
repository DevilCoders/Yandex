#pragma once

template <class Value>
class TSliceMap {
public:
    using TValueParam = typename TTypeTraits<Value>::TFuncParam;

public:
    TSliceMap(TValueParam value = Value()) {
        static_assert(N_SLICE_COUNT < ~ui16(0), "");
        SliceToValue.fill(value);
    }

    Value& operator[] (EFactorSlice slice) {
        return Ref(slice);
    }

    const Value& operator[] (EFactorSlice slice) const {
        return Get(slice);
    }

    Value& operator[] (const TStringBuf& sliceName) {
        return Ref(FromString<EFactorSlice>(sliceName));
    }

    const Value& operator[] (const TStringBuf& sliceName) const {
        return Get(FromString<EFactorSlice>(sliceName));
    }

    bool operator == (const TSliceMap<Value>& other) const {
        return SliceToValue == other.SliceToValue;
    }

    void Save(IOutputStream* rh) const {
        for (auto i : xrange(N_SLICE_COUNT)) {
            if (!(SliceToValue[i] == Value())) {
                ::Save(rh, ui16(i));
                ::Save(rh, SliceToValue[i]);
            }
        }
        ::Save(rh, Max<ui16>());
    }
    void Load(IInputStream* rh) {
        size_t i = 0;
        ui16 index = 0;
        while (i < N_SLICE_COUNT) {
            ::Load(rh, index);
            if (index == Max<ui16>()) {
                return;
            }
            Y_ASSERT(index < N_SLICE_COUNT);
            Y_ASSERT(i <= index);
            for (; i < index; ++i) {
                SliceToValue[i] = Value();
            }
            ::Load(rh, SliceToValue[index]);
            Y_ASSERT(index == i);
            i += 1;
        }
        ::Load(rh, index);
        Y_ASSERT(index == Max<ui16>());
    }

protected:
    Y_FORCE_INLINE Value& Ref(EFactorSlice slice) {
        Y_ASSERT(static_cast<size_t>(slice) < N_SLICE_COUNT);
        if (Y_LIKELY(static_cast<size_t>(slice) < N_SLICE_COUNT)) {
            return SliceToValue[static_cast<size_t>(slice)];
        }
        return SliceToValue[N_SLICE_COUNT];
    }

    Y_FORCE_INLINE const Value& Get(EFactorSlice slice) const {
        Y_ASSERT(static_cast<size_t>(slice) < N_SLICE_COUNT);
        if (Y_LIKELY(static_cast<size_t>(slice) < N_SLICE_COUNT)) {
            return SliceToValue[static_cast<size_t>(slice)];
        }
        return SliceToValue[N_SLICE_COUNT];
    }

private:
    std::array<Value, N_SLICE_COUNT + 1> SliceToValue;
};
