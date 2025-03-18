namespace NQueryExpansionsScheme;

struct TResourceState {
    Path (required): string;
    Revision (required): ui64;
};

struct TInputState {
    Resources: {string -> TResourceState};
    ScalarValues: {string -> any};
};
