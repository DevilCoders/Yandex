-- {{ file }} (file this query was created with)
$log = 'logs/awaps-log/{{ logs_scale }}';
$table_from = '{{ start.strftime(table_name_fmt) }}';
$table_to = '{{ end.strftime(table_name_fmt) }}';

$fc = (
    select
        placementid,
        SUM(cast(flight.amount_accepted as Double) / cast(flight.volume_accepted as Double)) as money
    from `home/inventory/ado_dictionaries/flight` as flight
    join `home/inventory/ado_dictionaries/campaign` as campaign on campaign.campaign_nmb = flight.campaign_nmb
    where
        campaign.agency_billing_id not in (2555468, 9660569, 6112941) and
        String::ToLower(campaign.advertiser_name) not like('%яндекс%') and
        String::ToLower(campaign.advertiser_name) not like('%yandex%') and
        NANVL(cast(flight.volume_accepted as Double), 0.0) > 0.0
    group by flight.awaps_placementid as placementid
);

$aab_requests = (
    select global_request_id, true as aab
    from range($log, $table_from, $table_to)
    where sectionid = '9001' and actionid = '15' and parameterstr like '%aadb=2%'
    group by global_request_id
);

select
    fielddate, actionid,
    count(*) as count,
    count_if(aab) as aab_count,
    count_if(money is null) as non_commercial_count,
    count_if(money is null and aab) as aab_non_commercial_count,
    coalesce(sum_if(money, actionid='0' and money is not null), 0) as money,
    coalesce(sum_if(money, actionid='0' and money is not null and aab), 0) as aab_money
from range($log, $table_from, $table_to) as log
left join $fc as fc on fc.placementid = cast(log.placementid as Uint64)
left join $aab_requests as aab_requests on log.global_request_id = aab_requests.global_request_id
where sectionid = '9001'
group by String::Substring(iso_eventtime, 0, 15) || '0:00' as fielddate, log.actionid as actionid
