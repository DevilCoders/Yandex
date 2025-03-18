-- {{ file }} (file this query was created with)
$log = 'logs/bs-chevent-log/{{ logs_scale }}';
$table_from = '{{ start.strftime(table_name_fmt) }}';
$table_to = '{{ end.strftime(table_name_fmt) }}';

$is_aab = ($adbbits) -> {return $adbbits is not NULL and $adbbits != '0';};
$turbo_pages = (select AsTuple(page_id, block_id) from `{{ pages_table }}`);
$is_turbo_page = ($pageid, $impid) -> {return AsTuple(cast($pageid as uint64), cast($impid as uint64)) in $turbo_pages;};

select
  fielddate, device, pageid,
  sum(if(fraudbits='0' and $is_aab(adbbits), cast(eventcost as uint64), 0)) * 30 / 1000000 / 1.18 as aab_money,
  sum(if(fraudbits='0', cast(eventcost as uint64), 0)) * 30 / 1000000 / 1.18 as money,
  count_if(countertype='1' and $is_aab(adbbits)) as aab_shows,
  count_if(countertype='1') as shows,
  count_if(countertype='2' and $is_aab(adbbits)) as aab_clicks,
  count_if(countertype='2') as clicks,
  count_if(countertype='1' and fraudbits!='0' and $is_aab(adbbits)) as aab_fraud_shows,
  count_if(countertype='2' and fraudbits!='0' and $is_aab(adbbits)) as aab_fraud_clicks,
  count_if(countertype='1' and fraudbits!='0') as fraud_shows,
  count_if(countertype='2' and fraudbits!='0') as fraud_clicks
from RANGE($log, $table_from, $table_to)
where placeid in ('542', '1542') and $is_turbo_page(pageid, impid)
group by
  pageid,
  String::Substring(iso_eventtime, 0, 15) || '0:00' as fielddate,
  if(cast(devicetype as uint64) > 4, 'desktop', 'mobile') as device
