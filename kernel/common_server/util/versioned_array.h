#pragma once
#include <util/system/types.h>
#include <util/generic/vector.h>

template <class T>
class TVersionedArray {
private:
    ui32 CurrentVersion = 0;
    struct TInternal {
        ui32 Version;
        T Data;
        TInternal(const ui32 version, const T& data)
            : Version(version)
            , Data(data)
        {

        }
    };
    TVector<TInternal> Data;
    T NullData = T();
public:

    void SetDefault(const T& nullData) {
        NullData = nullData;
    }

    void Resize(const ui32 size) {
        if (Data.size() != size) {
            Data.resize(size, TInternal(CurrentVersion - 1, NullData));
        }
    }

    void Clear() {
        ++CurrentVersion;
        if (CurrentVersion == 0) {
            CurrentVersion = 1;
            for (auto&& i : Data) {
                i.Version = 0;
                i.Data = T();
            }
        }
    }

    Y_FORCE_INLINE bool TouchData(const ui32 index) {
        Y_ASSERT(index < Data.size());
        if (Data[index].Version != CurrentVersion) {
            Data[index].Data = NullData;
            Data[index].Version = CurrentVersion;
            return true;
        }
        return false;
    }

    Y_FORCE_INLINE T& MutableData(const ui32 index) {
        TouchData(index);
        return Data[index].Data;
    }

    Y_FORCE_INLINE const T& GetData(const ui32 index) const {
        Y_ASSERT(index < Data.size());
        if (Data[index].Version != CurrentVersion) {
            return NullData;
        }
        return Data[index].Data;
    }

    Y_FORCE_INLINE bool HasData(const ui32 index) const {
        Y_ASSERT(index < Data.size());
        return (Data[index].Version == CurrentVersion);
    }

    ui32 Size() const {
        return Data.size();
    }
};

