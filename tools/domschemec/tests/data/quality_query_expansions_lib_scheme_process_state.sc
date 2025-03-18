namespace NQueryExpansionsScheme;

struct TTimePeriod {
    Year (required): ui32;
    Month: ui32;
    Day: ui32;
    Unit: string; // civil time unit, days by default
    Duration: ui32;
};

struct TYTDataTable {
    Type (required): string;
    Path (required): string;
};

struct TYTPeriodTable {
    Type (required): string;
    Path (required): string;
    Period (required): TTimePeriod;
};

struct TExpansionsSource {
    Type (required): string;
    RegionClass (required): string;
    RequestForm (required): string;
};

struct TYTLingBoostRequestsMapTable {
    Type (required): string;
    Path (required): string;
    KeyRequestForm (required): string;
    ValueRequestForm (required): string;
};

struct TYTLingBoostPeriodTable {
    Type (required): string;
    Path (required): string;
    Period (required): TTimePeriod;
    Sources (required): TExpansionsSource[];
    RequestsMapId: string;
};

struct TYTDataFile {
    Type (required): string;
    Path (required): string;
};

struct TProcessParams (relaxed) {
    Name (required): string;
    Smth: string;
};

struct TArtifact {
    Id: string; // optional id
    Role: string; // Input/Output

    DataTable: TYTDataTable;
    PeriodTable: TYTPeriodTable;
    LingBoostPeriodTable: TYTLingBoostPeriodTable;
    LingBoostRequestsMapTable: TYTLingBoostRequestsMapTable;

    DataFile: TYTDataFile;

    ProcessParams: TProcessParams;
};

struct TState {
    Artifacts (required): TArtifact[];
};
