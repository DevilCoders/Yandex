namespace NBlender::NProto;

struct TDuplicateRule {
    type (cppname = Type, required) : string;
    key (cppname = Key): string;
    args (cppname = Args) : any;
    prior (cppname = Priority) : i64 (default=0);
};
