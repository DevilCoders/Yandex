#pragma once

#include <util/system/yassert.h>

namespace NDoom {


template <class Accessor>
class TKeyDataFactory {
public:
    using TData = typename Accessor::TData;
    using TPosition = typename Accessor::TPosition;

    enum {
        Layers = Accessor::Layers
    };

    void Reset(const TPosition& position) {
        Data_ = TData();
        Layer_ = 0;
        Accessor::SetPosition(0, position, &Data_);
    }

    void AddHit() {
        Y_VERIFY(Layer_ < Layers);
        Accessor::AddHit(Layer_, &Data_);
    }

    void WriteLayer(const TPosition& position) {
        Y_VERIFY(Layer_ < Layers);
        ++Layer_;
        Accessor::SetPosition(Layer_, position, &Data_);
    }

    void FinishLayers(const TPosition& position) {
        if (Layer_ == Layers) {
            return;
        }
        while (Layer_ < Layers) {
            ++Layer_;
            Accessor::SetPosition(Layer_, position, &Data_);
        }
    }

    bool HasNonEmptyLayer() const {
        for (size_t layer = 0; layer < Layers; ++layer) {
            if (Accessor::Position(Data_, layer) != Accessor::Position(Data_, layer + 1)) {
                return true;
            }
        }
        return false;
    }

    const TData& Data() const {
        return Data_;
    }

private:
    TData Data_ = TData();
    size_t Layer_ = 0;
};


} // namespace NDoom
