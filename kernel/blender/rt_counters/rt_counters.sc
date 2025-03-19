namespace NBlender::NRtCounters;

struct TCalcParams {
    SurplusVersions : [string] (default = ["v6"]);
    Decays : [double] (default = [1, 30]);
};

struct TAggrSlice {
    Intent : string;
    PosRange : string;
    IsRandom : bool;
    Ui : string;
    SrpVer : string;
};

struct TMarker {
    AggrSlices : [TAggrSlice];
    CalcParams : TCalcParams;
};
