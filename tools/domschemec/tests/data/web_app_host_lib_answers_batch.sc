namespace NAppHost::NBatch;

struct THandlerSpec {
    name: string;
    path: string;
};

struct TSettings {
    handlers: [THandlerSpec];
    sequential: bool (default = false);
};

struct TInfo {
    prev_handler: string;
};
