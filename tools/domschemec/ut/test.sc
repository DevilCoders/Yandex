namespace NTest;

struct TOptionalA {
    a: string;
};

struct TRequiredA {
    a (required): string;
};

struct TAbc {
    abc: string;
};

struct TEmpty {
};

struct TEmptyStrict (strict) {
};

struct TEmptyRelaxed (relaxed) {
};

struct TDefaultABCD {
    a: i32 (default = 42);
    b: double (default = 0.42);
    c: string (default = "42");
    d: duration (default = "42ms");
};

struct TAnyA {
    a: any;
};

struct TDefaultAnyABC {
    a: any (default = 42);
    b: any (default = 0.42);
    c: any (default = "42");
};

struct TArrayA {
    a: [i32];
};

struct TDictA {
    a: { i32 -> string };
};

struct TDefaultArrayABC {
    a: [i32] (default = [4, 8, 15, 16, 23, 42]);
    b: [double] (default = [0.4, 0.8, 0.15, 0.16, 0.23, 0.42]);
    c: [string] (default = ["4", "8", "15", "16", "23", "42"]);

    e: [bool] (default = []);
};

struct TParentDefaultA {
    a: string (default = "parent");
};

struct TChildDefaultA: TParentDefaultA {
    a: string (default = "child");
};

struct TConstant {
    a: i64 (const = 123);
};
