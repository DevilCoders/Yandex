namespace NDismissCardRequestScheme;

struct TDismissCardRequest {
    card_id     : string (required);

    // for debug only
    time_now    : ui32; // epoch

    delete_card : bool (default = false);

    struct TAuth {
        uid : string (required, cppname = UID);
    };
    auth : TAuth;

    struct TAppMetrika {
        device_id : string (cppname = DeviceId);
        did       : string (cppname = DID);
        uuid      : string (cppname = UUID);
    };
    app_metrika : TAppMetrika;

    yandexuid : ui64 (cppname = YandexUID);
};
