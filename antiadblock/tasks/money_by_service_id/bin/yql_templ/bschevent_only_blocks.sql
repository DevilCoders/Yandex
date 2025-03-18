-- {{ file }} (file this query was created with)
$partners_blockids = (
  SELECT service_id, name, pageid, Cast(impid as String) as impid FROM (
    SELECT service_id, name, bn.0 as pageid, bn.1 as impids FROM (
        SELECT service_id, cn.0 as name, cn.1 as blocks FROM (
            SELECT column0.0 as service_id, column0.1 as custom_blockid_names FROM (
                SELECT Yson::ConvertTo(Yson::ParseJson('{{ service_id_to_block }}'), ParseType('Dict<String, Dict<String, Dict<String, List<Int64>>>>'))
            ) FLATTEN DICT BY column0
        ) FLATTEN BY custom_blockid_names as cn
    ) FLATTEN BY blocks as bn
  ) FLATTEN BY impids as impid
);

$log = "logs/bs-chevent-log/{{ logs_scale }}";
$table_from = '{{ start.strftime(table_name_fmt) }}';
$table_to = '{{ end.strftime(table_name_fmt) }}';

$is_aab = ($adbbits) -> {return $adbbits is not NULL and $adbbits != '0';};

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
  RANGE($log, $table_from, $table_to) as log
  join $partners_blockids as blocks on (log.pageid = blocks.pageid and log.impid = blocks.impid)
WHERE
  log.placeid in ('542', '1542')
GROUP BY
  log.producttype as `{{ columns.producttype }}`,
  blocks.service_id as `{{ columns.service_id }}`,
  blocks.name as `{{ columns.domain }}`,
  if(CAST(devicetype as uint64) > 4, 'desktop', 'mobile') as `{{ columns.device }}`,
  SUBSTRING(iso_eventtime, 0, 15) || '0:00' as `{{ columns.date }}`,
  $is_aab(adbbits) as `{{ columns.aab }}`,
  fraudbits != '0' as `{{ columns.fraud }}`;
