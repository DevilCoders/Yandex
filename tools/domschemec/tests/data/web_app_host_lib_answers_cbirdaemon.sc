namespace NImages::NCbirWebdaemon;

struct TCbirdaemonParams {
    signatures: [string];
    signatures_data: [string];
    image: string;
    user_crop: string;
};

struct TCbirdaemonResponse {
    signatures: {string -> string};
};
