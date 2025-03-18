namespace NBASSTanker;

struct TTankerResponse {
    struct TStation {
        companyId: string;
        name: string;
        city: string;
        regionId: i32;
        address: string;

        struct TFuel {
            id: string (required);
            name: string;
            marka: string;
        };
        fuels: [TFuel] (required);

        struct TLocation {
            lon : double (required);
            lat : double (required);
        };
        
        struct TColumn {
            fuels: [string];
            point: TLocation;
        };
        columns: { i32 -> TColumn } (required);

    };

    station: TStation (required);
};
