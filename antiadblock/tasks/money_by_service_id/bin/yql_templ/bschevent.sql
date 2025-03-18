-- {{ file }} (file this query was created with)
$partners_pageids = "{{ partners_pageids_table }}";
$log = "logs/bs-chevent-log/{{ logs_scale }}";
$table_from = '{{ start.strftime(table_name_fmt) }}';
$table_to = '{{ end.strftime(table_name_fmt) }}';

$is_aab = ($adbbits) -> {return $adbbits is not NULL and $adbbits != '0';};
$turbo_pages = (SELECT AsTuple(page_id, block_id) FROM `{{ turbo_pageid_impid_table }}`);
$is_turbo_page = ($pageid, $impid) -> {return AsTuple(cast($pageid as uint64), cast($impid as uint64)) in $turbo_pages;};

SELECT
  `{{ columns.service_id }}`,
  `{{ columns.producttype }}`,
  `{{ columns.domain }}`,
  `{{ columns.device }}`,
  `{{ columns.date }}`,
  `{{ columns.aab }}`,
  `{{ columns.fraud }}`,
  sum(CAST(eventcost as uint64)) * 30 / 1000000 / 1.18 as money,
  count_if(countertype='1') as shows,
  count_if(countertype='2') as clicks
FROM
  RANGE($log, $table_from, $table_to) as log join $partners_pageids as pages on log.pageid = pages.pageid
WHERE
  log.placeid in ('542', '1542') and not $is_turbo_page(log.pageid, log.impid)
GROUP BY
  log.producttype as `{{ columns.producttype }}`,
  pages.service_id as `{{ columns.service_id }}`,
  pages.name as `{{ columns.domain }}`,
  if(CAST(devicetype as uint64) > 4, 'desktop', 'mobile') as `{{ columns.device }}`,
  SUBSTRING(iso_eventtime, 0, 15) || '0:00' as `{{ columns.date }}`,
  $is_aab(adbbits) as `{{ columns.aab }}`,
  fraudbits != '0' as `{{ columns.fraud }}`;
