namespace NBassApi;

struct TGeoResponse {
    response(required): struct {
        GeoObjectCollection(required): struct {
            featureMember(required): [struct {
                GeoObject(required): struct {
                    metaDataProperty(required): struct {
                        GeocoderMetaData(required): struct {
                            Address(required): struct {
                                Components(required): [struct {
                                    kind(required): string;
                                    name(required): string;
                                }];
                            };
                            InternalToponymInfo(required): struct {
                                geoid(required): string;
                            };
                            text(required): string;
                        };
                    };
                };
            }];
        };
    };
};
struct TBuyerAddress {
    country : string;
    city: string;
    street: string;
    house: string;
    apartment: string;
    recipient: string;
    phone: string;
    text: string;
    regionId: ui64;
};
struct TBuyer {
    lastName: string;
    firstName: string;
    middleName: string;
    phone: string;
    email: string;
    dontCall: bool;
};
struct TDeliveryOption {
    id: string;
    deliveryOptionId: string;
    type: string;
    buyerPrice: ui64;
    dates: struct {
        fromDate: string;
        toDate: string;
    };
    deliveryIntervals: [struct {
        date: string;
        intervals: [struct {
            fromTime: string;
            toTime: string;
            isDefault: bool;
        }];
    }];
    paymentOptions: [struct {
        paymentType: string;
        paymentMethod: string;
    }];
};
struct TDeliveryDates {
    buyerPrice: ui64;
    fromDate: string;
    fromTime: string;
    isDefault: bool;
    optionId: string;
    toDate: string;
    toTime: string;
};
struct TDelivery {
    id: string;
    regionId: i64;
    buyerAddress: TBuyerAddress;
    dates: TDeliveryDates;
};
struct TCart {
    shopId: ui64;
    delivery: TDelivery;
    items: [struct {
        feedId: ui64;
        offerId: string;
        buyerPrice: double;
        showInfo: string;
        count: i32;
    }];
    deliveryOptions: [TDeliveryOption];
    notes: string;
};
struct TCheckouterOrder {
    id: ui64 (required);
    delivery: TDelivery (required);
    buyer: TBuyer (required);
};
struct TCheckouterData {
    buyerRegionId: i64;
    buyerCurrency: string;
    carts: [TCart];
    buyer: TBuyer;
};
struct TCheckoutRequest {
    buyerRegionId: i64;
    buyerCurrency: string;
    orders: [TCart];
    paymentMethod: string;
    paymentType: string;
    buyer: TBuyer;
};
struct TCheckoutResponse {
    checkedOut (required): i64;
    orders: [TCheckouterOrder];
};
// todo: MALISA-240 вынести TReportDefaultOfferBlue из этого файла
struct TReportOffer {
    titles(required): struct {
        raw(required): string;
    };
    categories(required): [struct {
        id: ui32;
    }];
    model (required): struct {
        id (required): ui32;
    };
    shop (required): struct {
        id (required): ui32;
        feed (required): struct {
            id (required): string;
            offerId (required): string;
        };
    };
    prices (required): struct {
        currency: string;
        value (required): string;
    };
    marketSku (required): string;
    wareId (required): string;
    feeShow (required): string;
    pictures: [struct {
        original: struct {
            url: string;
            height: ui64;
            width: ui64;
        };
        thumbnails: [struct {
            url: string;
            height: ui64;
            width: ui64;
        }];
    }];
    delivery: struct {
        options: [struct {
            price: struct {
                currency: string;
                value: string;
            };
            dayFrom: ui32;
        }];
    };
    warnings: struct {
        common: [struct {
            type: string;
            value: struct {
                full: string;
                short: string;
            };
        }];
    };
};
struct TReportDefaultOfferBlue {
    search (required): struct {
        results (required): [TReportOffer];
    };
};
// todo MALISA-240: повторяется много кода с другими респонсами, нужно их унифицировать
struct TReportSku {
    search (required): struct {
        results (required): [struct {
            product (required): struct {
                id (required): ui64;
            };
            offers (required): struct {
                items (required): [TReportOffer];
            };
        }];
    };
};
struct TMarketCheckoutState {
    step: string;
    attempt: ui32 (default = 0);
    attemptReminder : bool (default = false);
    muid: string;
    email: string;
    phone: string;
    buyerAddress: TBuyerAddress;
    defaultOffer: TReportOffer;
    deliveryOptions: [TDelivery];
    deliveryOptionIndex: ui32;
    sku: ui64;
    order: struct {
        checkouted_at_timestamp: ui64 (required);
        alice_id: string (required);
        attempt: ui32 (required);
        id: ui64;
    };
};
struct TCheckouterOrdersResponse {
    orders: [TCheckouterOrder] (required);
};
struct TCheckoutAuthRequest {
    ip: string;
    userAgent: string;
};
struct TCheckoutAuthResponse {
    muid: string;
};
