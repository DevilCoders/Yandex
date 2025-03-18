namespace NImages;

struct TMarketItem {
    I2TRelevance (required): double (default = 0.0);
    Title (required): string (default = "");
};

struct TMarketStorage {
    Items (required): {string -> TMarketItem};
};
