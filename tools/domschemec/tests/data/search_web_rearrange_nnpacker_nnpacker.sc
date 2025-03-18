namespace NNNPacker;

struct TLogOptions {
    Enabled : bool (default = false);
    Top : ui32 (default = 10);
    Groupings : {string -> string[]};
};

struct TNNPacker {
    LogOptions : TLogOptions;
};
