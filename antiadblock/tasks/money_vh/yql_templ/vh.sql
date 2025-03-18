-- {{ file }} (file this query was created with)
$vhmap_path = "{{ vh_map_path }}" || cast(CurrentUtcDate() as String);
$log = "logs/bs-dsp-log/{{ logs_scale }}";
$table_from = '{{ start.strftime(table_name_fmt) }}';
$table_to = '{{ end.strftime(table_name_fmt) }}';

$is_aab = ($adbbits) -> {return $adbbits is not NULL and $adbbits != '0';};
$time = ($ts) -> {return $ts - $ts % 600;};
$vhmap = (select vh_category_id, vh_category_name, PageID, ImpID from $vhmap_path where is_vh='vh');

select
  ts, category_id, device,
  count_if(countertype='0' and $is_aab(adbbits)) as aab_hits,
  count_if(countertype='0') as hits,
  count_if(countertype='0' and win='1' and $is_aab(adbbits)) as aab_wins,
  count_if(countertype='0' and win='1') as wins,
  count_if(countertype='1' and dspfraudbits='0' and $is_aab(adbbits)) as aab_shows,
  count_if(countertype='1' and dspfraudbits='0') as shows,
  sum(if(countertype='1' and dspfraudbits='0' and $is_aab(adbbits), cast(price as uint64), 0)) / 1000000. as aab_money,
  sum(if(countertype='1' and dspfraudbits='0', cast(price as uint64), 0)) / 1000000. as money
from
  RANGE($log, $table_from, $table_to) as logs join $vhmap as map on map.PageID = cast(logs.pageid as uint64) and map.ImpID = cast(logs.impid as uint64)
group by
  $time(cast(unixtime as uint32)) as ts,
  map.vh_category_id as category_id,
  if(CAST(devicetype as uint64) > 4, 'desktop', 'mobile') as device
