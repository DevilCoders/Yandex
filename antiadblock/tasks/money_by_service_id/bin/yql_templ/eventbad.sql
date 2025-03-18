-- {{ file }} (file this query was created with)
$partners_pageids = "{{ partners_pageids_table }}";
$log = "logs/bs-proto-eventbad-log/{{ logs_scale }}";
$table_from = '{{ start.strftime(table_name_fmt) }}';
$table_to = '{{ end.strftime(table_name_fmt) }}';

$is_aab = ($adbbits) -> {return $adbbits is not NULL and $adbbits != 0;};

SELECT
  `{{ columns.service_id }}`,
  `{{ columns.producttype }}`,
  `{{ columns.domain }}`,
  `{{ columns.device }}`,
  `{{ columns.date }}`,
  `{{ columns.aab }}`,
  sum(if(EventCost is not null, EventCost, 0)) * 30 / 1000000 / 1.18 as {{ columns.money }},
  count_if(CounterType=1) as {{ columns.shows }},
  count_if(CounterType=2) as {{ columns.clicks }}
FROM
  RANGE($log, $table_from, $table_to) as log join $partners_pageids as pages on log.PageID = CAST(pages.pageid as int64)
WHERE
  log.PlaceID in (542, 1542)
GROUP BY
  log.ProductType as `{{ columns.producttype }}`,
  pages.service_id as `{{ columns.service_id }}`,
  pages.name as `{{ columns.domain }}`,
  if(log.DeviceType > 4, 'desktop', 'mobile') as `{{ columns.device }}`,
  SUBSTRING(log.iso_eventtime, 0, 15) || '0:00' as `{{ columns.date }}`,
  $is_aab(log.AdbBits) as `{{ columns.aab }}`;
