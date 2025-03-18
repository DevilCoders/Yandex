namespace NOlympiadBroRule;

struct TOlympiadBro {
    Enabled : bool(default = false);
    TargetGrouping : string (default = "d");
    WizardGrouping : string (default = "wizplace");
    CacheDuration : ui64 (default = 120);
    CachePrefix : string (default = "OlympiadBro");
    SerpInfoDetectKey : string (default = "type");
    SerpInfoDetectVal : string (default = "sport/olympiad");
    SerpDataKeyPath : string (default = "parent_collection/ads");
    YaBroClids : {string -> bool};
    ClidProp : string (default = "Banner.stat/0/clid_type.nodump");
    TextChoices : [string];
    BroUrl : string (default = "https://browser.yandex.ru/download/?partner_id=WG2018");
    TextIdx : ui32;
};
