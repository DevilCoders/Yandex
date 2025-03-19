#pragma once

#include "rect.h"

template <class T>
class TFilteredObjects {
public:
    TFilteredObjects(ui32 filterDistance, const TRect<TGeoCoord>* searchRect = nullptr)
        : FilterDistance(filterDistance)
        , MainRect(searchRect) {}

    void AddPoint(const T& object) {
        for (ui32 i = 0; i < Objects.size(); ++i) {
            T& p = Objects[i];
            if (p.Coord.GetLengthTo(object.Coord) < FilterDistance) {
                if (!!MainRect && object.Coord.X < p.Coord.X) {
                    p = object;
                }
                return;
            }
        }
        if (!MainRect || MainRect->Contain(object.Coord)) {
            Objects.push_back(object);
        }
    }

    ui32 GetSize() const {
        return Objects.size();
    }

    const TVector<T>& GetObjects() const {
        return Objects;
    }
private:
    double FilterDistance = 0;
    const TRect<TGeoCoord>* MainRect;
    TVector<T> Objects;
};
