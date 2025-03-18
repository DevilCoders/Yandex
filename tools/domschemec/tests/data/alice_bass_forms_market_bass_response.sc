namespace NBassApi;

struct TFormUpdate {
    name: string (required);
    slots: [any] (required);
};

struct TPrice {
    value: string (required);
    currency: string;
};

struct TPicture {
    height: ui64 (required);
    width: ui64 (required);
    url: string (required);
};

struct TWarning {
    type: string (required);
    value: string (required);
};

// todo MALISA-240 разнести схемы нанеймспейсы
struct TOutputDelivery {
    date: string (required);
    prices: TPrice (required);
};

struct TBeruOrderCardData {
    title: string (required);
    prices: TPrice (required);
    action (required): struct {
        form_update: TFormUpdate (required);
    };
    picture: TPicture (required);
    urls (required): struct {
        terms_of_use: string (required);
        model: string (required);
        supplier: string (required);
    };
    delivery: TOutputDelivery (required);
};

struct TCheckoutAskPhoneTextCardData {
    is_guest: bool (required);
};
