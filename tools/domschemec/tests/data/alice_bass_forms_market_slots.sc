namespace NBassApi;

struct TProductSlot {
    type: string (required, allowed=["model", "offer", "sku"]);
    id: ui64;
    ware_id: string;
    gl_filters: {string -> [string]};

    (validate id) {
        if (Type() == "model"sv && !HasId()) {
            AddError("\"id\" field is necessary for type \"model\"");
        }
        if (Type() == "sku"sv && !HasId()) {
            AddError("\"id\" field is necessary for type \"sku\"");
        }
    };
    (validate ware_id) {
        if (Type() == "offer"sv && !HasWareId()) {
            AddError("\"ware_id\" field is necessary for type \"offer\"");
        }
    };
};
