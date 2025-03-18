namespace NAppHost::NWeb;

struct TRequest {
    uri: string;
    tld: string;
    time: string;
    scheme: string;
    referer_is_ya: string;
    referer: string;
    proto: string;
    path: string;
    params: {string -> [string]};
    is_suspected_robot: string;
    is_internal: string;
    ip: string;
    hostname: string;
    headers: {string -> string};
    gsmop: string;
    cookies: string;
};

struct TBrowserField {
    engine: string;
    engineVersion: string;
    name: string;
    version: string;
};

struct TDeviceField {
    id: string;
    model: string;
    name: string;
    vendor: string;
};

struct TOsField {
    family: string;
    version: string;
};

struct TDevice {
    browser: TBrowserField;
    device: TDeviceField;
    os: TOsField;
};

struct TCountryField {
    id: i32;
    name: string;
    path: [i32];
};

struct TRegionField {
    country: TCountryField;
    id: i32;
    name: string;
    path: [i32];
};

struct TGeoLocationField {
    precision: i32;
    region_id_by_ip: i32;
};

struct TRegion {
    geolocation: TGeoLocationField;
    is_manual: string;
    default: TRegionField;
    real: TRegionField;
    selected: TRegionField;
    tuned: TRegionField;
};

struct TFlags {
    all: {string -> any};
};

struct TExperiments {
    contexts: {string -> {string -> any}};
    ids: [string];
    slots: [[string]];
};

struct TSettings {
    external: {string -> any};
    internal: {string -> any};
    filter: i32;
};

struct TUser {
    fuid: string;
    id: string;
    login: string;
    ruid: string;
    uuid: string;
};

struct TAccess {
    is_trusted_net: bool;
    is_yandex_net: bool;
};

struct TReport {
    ajax: {string -> string};
    clid: string;
    device: string;
    is_advanced: bool;
    language: string;
    lr: TRegionField;
    name: string;
    params: {string -> any};
    project: string;
    reqid: string;
    restrictions: {string -> any}; //XXX
    serp_version: string;
    text: string;
    user_text: string;
};
