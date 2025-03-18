namespace NBassApi;

struct TRecipientInfo {
    title               : string (required, cppname = Title);
    phone               : string (required, cppname = Phone);  // to display
    phone_uri           : string (required, cppname = PhoneUri);  // to actually make a call
    type                : string (required, cppname = Type, allowed = ["emergency", "direct", "contact"]);
};

