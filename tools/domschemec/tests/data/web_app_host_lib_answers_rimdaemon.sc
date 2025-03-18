namespace NImages::NRimWebdaemon;

struct TRimdaemonParams {
    rim_id: string;
    namespace: string;
    lang: string;
    doc_count: ui32;
    link_count: ui32;
    big_thumbs: bool;
    exp_links: bool;
    exp_params: string;
};

struct TDocDescription {
    iu: string;
    iw: ui32;
    ih: ui32;
    pu: string;
    ph: string;
    t: string;
};

struct TRelatedDoc {
    id: string;
    zid: string;
    r: double;
    tid: string;
    tw: ui32;
    th: ui32;
    bid: string;
    btw: ui32;
    bth: ui32;
    bdr: string;
    s: [TDocDescription];
};

struct TRimResponse {
    id: string;

    rld: [TRelatedDoc];
};
