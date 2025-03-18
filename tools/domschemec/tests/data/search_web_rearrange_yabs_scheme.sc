namespace NYabs;

struct TYabs {
    BaseSearchFlags : {string -> string};
    UseDupHost : bool (default = false);
    RandomType : bool (default = false);
    Fml : string (default = "");
    FilterFml : string (default = "");
    FilterThreshold : double (default = 0.0);
};
