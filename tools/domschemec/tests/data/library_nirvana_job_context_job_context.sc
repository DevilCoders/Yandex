namespace NNirvana;

struct TMeta {
    workflowUid (required): string;
    workflowURL (cppname = WorkflowUrl, required): string;
    operationUid (required): string;
    blockUid: string;
    blockURL: string;
    blockCode: string;
    processUid: string;
    processURL (cppname = ProcessUrl): string;
    description: string;
    owner (required): string;
};

struct TPorts {
    udp: { string -> ui32 };
    tcp: { string -> ui32 };
};

struct TEndpoint {
    dataType (required): string;
    wasUnpacked: bool;
    unpackedDir: string;
    unpackedFile: string;
};

struct TStatus {
    errorMsg: string;
    successMsg: string;
};

struct TJobContextData {
    meta (required): TMeta;
    parameters: { string -> any };

    ports: TPorts;

    inputs: { string -> string[] };
    outputs: { string -> string[] };

    inputItems: { string -> TEndpoint[] };
    outputItems: { string -> TEndpoint[] };
    status: TStatus;
};

// This is kept in the same file because .sc does not support including/dependencies
struct TMrPath {
    path: string;
    rawPath: string;
    type: string;
    cluster: string;
    server: string;
    host: string;
    port: ui32;
};

struct TMrCluster {
    name: string;
    server: string;
    host: string;
    port: ui32;
};

struct TMrJobContextData: TJobContextData {
    mrCluster: TMrCluster;
    mrInputs: { string -> TMrPath[] };
    mrOutputs: { string -> TMrPath[] };
    mrTmp: TMrPath;
    mapreduce: string;
};
