namespace NAppHost::NLaas;

struct TLaasRegion {
    region_id: i32 (required);
    city_id: i32 (required);
    region_by_ip: i32 (required);
    precision: i32 (required);
    latitude: double (required);
    longitude: double (required);
    location_accuracy: i64 (required);
    location_unixtime: i64 (required);

    should_update_cookie: bool (required);
    is_user_choice: bool (required);

    suspected_region_city: i32 (required);
    suspected_latitude: double (required);
    suspected_longitude: double (required);
    suspected_location_accuracy: i64 (required);
    suspected_location_unixtime: i64 (required);
    suspected_precision: i32 (required);

    probable_regions_reliability: double;
    probable_regions: {i32 -> double};
    isp_code: i64 (required);

    gsm_operator_if_ip_is_mobile: string;
    country_id_by_ip: i32;
    is_anonymous_vpn: bool;
    is_public_proxy: bool;
    is_serp_trusted_net: bool;
    is_tor: bool;
    is_hosting: bool;
    is_gdpr: bool;
    is_mobile: bool;
    is_yandex_net: bool;
    is_yandex_staff: bool;
};
