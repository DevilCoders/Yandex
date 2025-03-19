#pragma once

#include <kernel/geo/types/region_types.h>

#include <library/cpp/deprecated/small_array/small_array.h>

#include <util/generic/vector.h>
#include <util/system/yassert.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>


#pragma pack(1)
template<class TGeoRegionParam, class TDataParam>
struct Y_PACKED TTRegData
{
    typedef TGeoRegionParam TGeoRegion;
    typedef TDataParam      TData;

    TGeoRegion Reg;
    TData      Data;
};
#pragma pack()

template<class TGeoRegionParam, class TDataParam>
class TSerializer<TTRegData<TGeoRegionParam, TDataParam>>:
    public TSerializerTakingIntoAccountThePodType<TTRegData<TGeoRegionParam, TDataParam>, true> {
};

template<class TGeoRegion, class TData>
inline TTRegData<TGeoRegion,TData> MakeTRegData(TGeoRegion reg, TData data)
{
    TTRegData<TGeoRegion,TData> rd;
    rd.Reg  = reg;
    rd.Data = data;
    return rd;
}

template<class TGeoRegion, class TData>
inline IOutputStream& operator<<(IOutputStream& o, const TTRegData<TGeoRegion,TData>& t)
{
    o << '[' << ui64(t.Reg) << " (" << t.Data << ")]";
    return o;
}
