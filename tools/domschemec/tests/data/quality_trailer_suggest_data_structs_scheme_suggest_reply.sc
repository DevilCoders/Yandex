namespace NSuggestReply;

struct TTyped {
    type (required) : string;
    subtype (required) : string (default = "");
};

struct TWeighted {
    value (required) : string;
    weight : double;
};

struct TTitle {
    value (required) : string;
};

struct TSuggestion : TTyped {
    query (required) : TWeighted;
    title (required) : TTitle;
    subtitle : TTitle;
    source (required) : string (default = "Null");
};

struct TTheme {
    backgroudColor (required) : string;
    appearance (required) : string;
    textColor : string;
    actionColor : string;
};

struct TNavigatable {
    url (required) : string;
    image : string;
    omnibox : bool;
    theme : TTheme;
};

struct TNavigation : TSuggestion {
    navigation : TNavigatable;
};

struct THistoriable {
    device : string;
    timestamp : ui32;
};

struct THistory : TSuggestion {
    history : THistoriable;
};

struct TGroup : TTyped {
    content (required) : TSuggestion[];
    title : string;
    weight : double;
};

struct TSuggestReply : TTyped {
    query (required) : TWeighted;
    autocompleteQuery : TWeighted;

    prefetchQuery : TWeighted;
    prefetchUrl : TWeighted;

    content (required) : TGroup[];
};
