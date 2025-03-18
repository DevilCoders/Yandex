namespace NRecommender::NSCProto;

struct TRecommendation {
    url (cppname = Url, required) : string;
    logurl (cppname = LogUrl) : string;
    turbourl (cppname = TurboUrl) : string;
    host (cppname = Host, required) : string;
    title (cppname = Title) : string;
    thumb (cppname = Thumb) : string;
    annotation (cppname = Annotation) : string;
    relevance (cppname = Relevance) : double;
    relev_predict (cppname = RelevPredict) : double;
    model (cppname = Model) : string;
    source (cppname = Source) : string;
    images (cppname = Images) : {string -> string};
    factors (cppname = Factors) : string;
    hash (cppname = Hash) : string;
};

struct TDoc {
    url (cppname = Url, required) : string;
    doctitle (cppname = Doctitle, required) : string (default = "RECOMMEND");
    articleness (cppname = Articleness) : double;
    show_classifier_score (cppname = ShowClassifierScore) : double;
    construct (cppname = Construct, required) : {string -> any};
};
