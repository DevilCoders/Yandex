USE hahn;
PRAGMA Library('tables.sql');
IMPORT tables SYMBOLS $last_n_tables;


$features_table = "//home/cloud_analytics/tmp/antifraud/account_name_dist";
$result_table = "//home/cloud_analytics/tmp/antifraud/passport_reg";
$cloud_accounts_path = "//home/logfeller/logs/yc-billing-export-billing-accounts/1h";


$cloud_accounts = (
       select distinct
              billing_account_id,  
              registration_ip,
              Ip::GetSubnet(Ip::FromString(registration_ip)) as registration_subnet,
              Geo::RoundRegionByIp(registration_ip, "country").en_name as registration_country,
              Geo::RoundRegionByIp(registration_ip, "city").en_name as registration_city,
              Geo::RoundRegionByIp(registration_ip, "region").en_name as registration_region,
              Geo::RoundRegionByIp(registration_ip, "district").en_name as registration_district,
              Geo::GetIspNameByIp(registration_ip) as registration_isp,
              Geo::GetAsset(registration_ip) as registration_asn,
              Geo::GetOrgNameByIp(registration_ip) as registration_org,
              Geo::IsYandexStaff(registration_ip) as yandex_staff,
              Geo::IsMobile(registration_ip) AS is_mobile_ip,
              Geo::IsHosting(registration_ip) AS is_hosting_ip,
              Geo::IsProxy(registration_ip) AS is_proxy_ip,
              Geo::IsVpn(registration_ip) AS is_vpn_ip,
              Geo::IsTor(registration_ip) AS is_tor_ip,
              Ip::IsIPv4(registration_ip) AS is_ipv4,
              Ip::IsIPv6(registration_ip) AS is_ipv6


              
       from
       (
              SELECT id as billing_account_id,
                     cast(owner_id as uint64) AS passport_id,
                     Yson::LookupString(Yson::ParseJson(Yson::ConvertToString(metadata)), 
                                   'registration_ip', Yson::Options(false as Strict)) as registration_ip
              FROM $last_n_tables($cloud_accounts_path, 1) 
              where id is not null
       ));

INSERT INTO $result_table WITH TRUNCATE  
select *
from $features_table as ft
left join $cloud_accounts as ca on ft.billing_account_id = ca.billing_account_id

